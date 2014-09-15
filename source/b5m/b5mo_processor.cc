#include <idmlib/b5m/b5mo_processor.h>
#include <idmlib/b5m/b5m_types.h>
#include <idmlib/b5m/b5m_helper.h>
#include <idmlib/b5m/b5m_mode.h>
#include <idmlib/b5m/scd_doc_processor.h>
#include <sf1common/Utilities.h>
#include <idmlib/b5m/product_price.h>
#include <am/sequence_file/ssfr.h>
#include <util/filesystem.h>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <glog/logging.h>

using namespace idmlib::b5m;

B5moProcessor::B5moProcessor(const B5mM& b5mm)
:b5mm_(b5mm), odb_(NULL), matcher_(NULL), sorter_(NULL), omapper_(NULL), mode_(0)
,attr_(NULL), tag_extractor_(NULL), rank_filter_(NULL), maxent_title_(NULL), sp_(NULL)
{
    status_.AddCategoryGroup("^电脑办公.*$","电脑办公", 0.001);
    status_.AddCategoryGroup("^手机数码>手机$","手机", 0.001);
    status_.AddCategoryGroup("^手机数码>摄像摄影.*$","摄影", 0.001);
    status_.AddCategoryGroup("^家用电器.*$","家用电器", 0.001);
    status_.AddCategoryGroup("^服装服饰.*$","服装服饰", 0.0001);
    status_.AddCategoryGroup("^鞋包配饰.*$","鞋包配饰", 0.0001);
    status_.AddCategoryGroup("^运动户外.*$","运动户外", 0.0001);
    status_.AddCategoryGroup("^母婴童装.*$","母婴童装", 0.0001);
}

B5moProcessor::~B5moProcessor()
{
    if(odb_!=NULL) delete odb_;
    if(matcher_!=NULL) delete matcher_;
    if(tag_extractor_!=NULL) delete tag_extractor_;
    if(rank_filter_!=NULL) delete rank_filter_;
    if(maxent_title_!=NULL) delete maxent_title_;
    if(sp_!=NULL) delete sp_;
}
void B5moProcessor::LoadMobileSource(const std::string& file)
{
    std::ifstream ifs(file.c_str());
    std::string line;
    while( getline(ifs,line))
    {
        boost::algorithm::trim(line);
        mobile_source_.insert(line);
    }
    ifs.close();
}

void B5moProcessor::Process(ScdDocument& doc)
{
    SCD_TYPE& type = doc.type;
    //reset type
    if(mode_!=B5MMode::INC)
    {
        if(type==DELETE_SCD)
        {
            type = NOT_SCD;
        }
        else
        {
            type = UPDATE_SCD;
        }
    }
    else
    {
        if(type==INSERT_SCD)
        {
            type = UPDATE_SCD;
        }
    }
    if(type==NOT_SCD) return;
    std::string sdocid;
    doc.getString("DOCID", sdocid);
    if(sdocid.length()!=32)
    {
        type = NOT_SCD;
        return;
    }

    if(type==UPDATE_SCD&&rank_filter_!=NULL)
    {
        if(rank_filter_->Get(sdocid))
        {
            doc.property(B5MHelper::RawRankPN()) = (int64_t)10000;
        }
    }

    //format Price property
    Document::doc_prop_value_strtype uprice;
    if(doc.getProperty("Price", uprice))
    {
        ProductPrice pp;
        pp.Parse(propstr_to_ustr(uprice));
        doc.property("Price") = pp.ToPropString();
    }

    //set mobile tag
    Document::doc_prop_value_strtype usource;
    if(doc.getString("Source", usource))
    {
        std::string ssource = propstr_to_str(usource);
        if(mobile_source_.find(ssource)!=mobile_source_.end())
        {
            doc.property("mobile") = (int64_t)1;
        }
    }


    //set pid(uuid) tag

    uint128_t oid = B5MHelper::StringToUint128(sdocid);
    sdocid = B5MHelper::Uint128ToString(oid); //to avoid DOCID error;
    std::string spid;
    std::string old_spid;
    if(type==RTYPE_SCD)
    {
        //boost::shared_lock<boost::shared_mutex> lock(mutex_);
        if(odb_->get(sdocid, spid)) 
        {
            doc.property("uuid") = str_to_propstr(spid);
            type = UPDATE_SCD;
            ProcessIU_(doc);
        }
        else
        {
            //LOG(ERROR)<<"rtype docid "<<sdocid<<" does not exist"<<std::endl;
            type = NOT_SCD;
        }
    }
    else if(type!=DELETE_SCD) //I or U
    {
        if(omapper_!=NULL)
        {
            doc.eraseProperty("Category");
        }
        ProcessIU_(doc);
    }
    else
    {
        doc.clearExceptDOCID();
        //boost::shared_lock<boost::shared_mutex> lock(mutex_);
        odb_->get(sdocid, spid);
        old_spid = spid;
        if(spid.empty())
        {
            type=NOT_SCD;
            //LOG(INFO)<<"DELETE pid empty : "<<sdocid<<std::endl;
        }
        else
        {
            doc.property("uuid") = str_to_propstr(spid);
            ScdDocument sdoc(doc, DELETE_SCD);
            sorter_->Append(sdoc, ts_);
        }
    }
    if(!changed_match_.empty())
    {
        changed_match_.erase(oid);
    }
}
void B5moProcessor::PendingProcess_(ScdDocument& doc)
{
    std::string spid;
    doc.getString("uuid", spid);
    if(spid.empty())
    {
        Product p;//empty p
        ProcessIUProduct_(doc, p);
    }
    else
    {
        Product p;
        p.type = Product::FASHION;
        p.spid = spid;
        ProcessIUProduct_(doc, p);
    }
    std::string doctext;
    ScdWriter::DocToString(doc, doctext);
    boost::unique_lock<boost::shared_mutex> lock(mutex_);
    writer_->Append(doctext, doc.type);
}
void B5moProcessor::ProcessIUProduct_(ScdDocument& doc, Product& product, const std::string& old_spid)
{
    if(product.type==Product::PENDING)
    {
        doc.type = NOT_SCD;
        return;
    }
    if(product.type==Product::SPU&&!product.stitle.empty())
    {
        //if(boost::algorithm::starts_with(product.scategory, "美容美妆"))
        //{
        //    spumatch_count_.fetch_add(1);
        //}
        spumatch_count_.fetch_add(1);
        //std::cerr<<"[M]"<<title<<","<<product.stitle<<"\t["<<sdocid<<","<<scategory<<","<<product.spid<<"]"<<std::endl;
    }
    SCD_TYPE type = UPDATE_SCD;
    std::string sdocid;
    doc.getString("DOCID", sdocid);
    std::string title;
    doc.getString("Title", title);
    std::string scategory;
    doc.getString("Category", scategory);
    std::string spid;
    //if(product.type==Product::BOOK)
    //{
    //    doc.property(B5MHelper::GetProductTypePropertyName()) = str_to_propstr("BOOK");
    //}
    //else if(product.type==Product::SPU)
    //{
    //    doc.property(B5MHelper::GetProductTypePropertyName()) = str_to_propstr("SPU");
    //}
    //else if(product.type==Product::FASHION)
    //{
    //    doc.property(B5MHelper::GetProductTypePropertyName()) = str_to_propstr("FASHION");
    //}
    //else if(product.type==Product::ATTRIB)
    //{
    //    doc.property(B5MHelper::GetProductTypePropertyName()) = str_to_propstr("ATTRIB");
    //}
    //else
    //{
    //    doc.property(B5MHelper::GetProductTypePropertyName()) = str_to_propstr("NOTP");
    //}
    std::string original_attribute;
    doc.getString("Attribute", original_attribute);
    if(!product.spid.empty())
    {
        spid = product.spid;
        if(!title.empty()) 
        {
            if(product.type==Product::SPU)
            {
                std::vector<Attribute> eattributes;
                std::string sattrib;
                doc.getString("Attribute", sattrib);
                boost::algorithm::trim(sattrib);
                if(!sattrib.empty())
                {
                    UString uattrib(sattrib, UString::UTF_8);
                    ProductMatcher::ParseAttributes(uattrib, eattributes);
                }
                if(!product.display_attributes.empty()||!product.filter_attributes.empty())
                {
                    std::vector<Attribute> v;
                    ProductMatcher::ParseAttributes(product.filter_attributes, v);
                    ProductMatcher::MergeAttributes(eattributes, v);
                    v.clear();
                    ProductMatcher::ParseAttributes(product.display_attributes, v);
                    ProductMatcher::MergeAttributes(eattributes, v);
                }
                else
                {
                    ProductMatcher::MergeAttributes(eattributes, product.attributes);
                    ProductMatcher::MergeAttributes(eattributes, product.dattributes);
                }
                UString new_uattrib = ProductMatcher::AttributesText(eattributes);
                std::string new_sattrib;
                new_uattrib.convertString(new_sattrib, UString::UTF_8);
                try {
                    new_sattrib = attr_->attr_normalize(new_sattrib);
                }
                catch(std::exception& ex)
                {
                    //TODO do nothing, keep new_sattrib not change
                }
                doc.property("Attribute") = str_to_propstr(new_sattrib);
            }
        }
        //match_ofs_<<sdocid<<","<<spid<<","<<title<<"\t["<<product.stitle<<"]"<<std::endl;
    }
    else
    {
        spid = sdocid;
    }
    std::string knn;
    doc.getString("KNN", knn);
    if(!knn.empty())
    {
        std::string sattrib;
        doc.getString("Attribute", sattrib);
        if(!sattrib.empty())
        {
            sattrib = sattrib+",KNN:"+knn;
        }
        else
        {
            sattrib = "KNN:"+knn;
        }
        doc.property("Attribute") = str_to_propstr(sattrib);
        doc.eraseProperty("KNN");
    }
    if(!product.stitle.empty() && !title.empty())
    {
        doc.property(B5MHelper::GetSPTPropertyName()) = str_to_propstr(product.stitle);
    }
    if(!scategory.empty()) 
    {
        scategory+=">";
        doc.property("Category") = str_to_propstr(scategory);
    }
    if(!title.empty()&&tag_extractor_!=NULL)
    {
        std::vector<std::string> tags;
        try {
            tags = tag_extractor_->get_tags(title);
        }
        catch(std::exception& ex)
        {
            LOG(ERROR)<<"get tags error for "<<title<<std::endl;
        }
        if(!tags.empty())
        {
            std::stringstream ss;
            for(std::size_t i=0;i<tags.size();i++)
            {
                if(i>0) ss<<",";
                ss<<tags[i];
            }
            std::string tag_str = ss.str();
            if(!tag_str.empty())
            {
                doc.property(B5MHelper::GetTagsPropertyName()) = str_to_propstr(tag_str);
            }
        }
    }
    if(!product.spic.empty() && !title.empty())
    {
        //TODO remove this restrict
        std::vector<std::string> spic_vec;
        boost::algorithm::split(spic_vec, product.spic, boost::algorithm::is_any_of(","));
        if(spic_vec.size()>1)
        {
            product.spic = spic_vec[0];
        }
        doc.property(B5MHelper::GetSPPicPropertyName()) = str_to_propstr(product.spic);
    }
    if(!product.surl.empty() && !title.empty())
    {
        doc.property(B5MHelper::GetSPUrlPropertyName()) = str_to_propstr(product.surl);
    }
    if(!product.smarket_time.empty() && !title.empty())
    {
        doc.property("MarketTime") = str_to_propstr(product.smarket_time);
    }
    if(!product.sbrand.empty() && !title.empty())
    {
        doc.property(B5MHelper::GetBrandPropertyName()) = str_to_propstr(product.sbrand);
    }
    if(!product.sub_prop.empty() && !title.empty())
    {
        doc.property(B5MHelper::GetSubPropPropertyName()) = str_to_propstr(product.sub_prop);
    }
    doc.property("uuid") = str_to_propstr(spid);
    if(old_spid!=spid)
    {
        odb_->insert(sdocid, spid);
    }
    if(old_spid!=spid&&!old_spid.empty())
    {
        ScdDocument old_doc;
        old_doc.property("DOCID") = str_to_propstr(sdocid, UString::UTF_8);
        old_doc.property("uuid") = str_to_propstr(old_spid, UString::UTF_8);
        old_doc.type=DELETE_SCD;
        sorter_->Append(old_doc, ts_, 1);
    }
    ScdDocument sdoc(doc, type);
    sorter_->Append(sdoc, ts_);
    if(spid==old_spid)
    {
        doc.eraseProperty("uuid");
    }
}
void B5moProcessor::EstimateSA_(ScdDocument& doc)
{
    std::size_t count = 0;
    std::string scount;
    doc.getString(B5MHelper::GetCommentCountPropertyName(), scount);
    if(!scount.empty())
    {
        try {
            count = boost::lexical_cast<std::size_t>(scount);
        }
        catch(std::exception& ex)
        {
            count = 0;
        }
    }
    if(count==0&&mode_>0&&!b5mm_.comment_ip.empty())
    {
        std::string docid;
        doc.getString("DOCID", docid);
        msgpack::rpc::session se = sp_->get_session(b5mm_.comment_ip, b5mm_.comment_port);
        uint32_t icount = se.call("get",docid).get<uint32_t>();
        count = icount;
        if(count!=0)
        {
            //std::cerr<<"[C]"<<docid<<","<<count<<std::endl;
            doc.property(B5MHelper::GetCommentCountPropertyName()) = boost::lexical_cast<std::string>(count);
        }
    }
    std::string source;
    doc.getString("Source", source);
    if(count>0&&!source.empty()&&source!="淘宝网"&&source!="天猫")
    {
        std::string samount;
        doc.getString(B5MHelper::GetSalesAmountPropertyName(), samount);
        if(samount.empty())
        {
            int64_t sa = B5MHelper::CommentCountToSalesAmount(count);
            doc.property(B5MHelper::GetSalesAmountPropertyName()) = 
              str_to_propstr(boost::lexical_cast<std::string>(sa));
        }
    }
}

void B5moProcessor::ProcessIU_(ScdDocument& doc, bool force_match)
{
    //SCD_TYPE type = UPDATE_SCD;
    doc.eraseProperty(B5MHelper::GetSPTPropertyName());
    doc.eraseProperty(B5MHelper::GetSPUrlPropertyName());
    doc.eraseProperty(B5MHelper::GetSPPicPropertyName());
    doc.eraseProperty("FilterAttribute");
    doc.eraseProperty("DisplayAttribute");
    doc.eraseProperty(B5MHelper::GetBrandPropertyName());
    std::string category;
    doc.getString("Category", category);
    if(category.empty()&&omapper_!=NULL)
    {
        OMap_(*omapper_, doc);
    }
    category.clear();
    doc.getString("Category", category);
    EstimateSA_(doc);
    //{
    //    if(category.empty())
    //    {
    //        uint32_t debug2 = debug2_.fetch_add(1);
    //        if(debug2%100000==0)
    //        {
    //            LOG(INFO)<<"un-categorized "<<debug2<<std::endl;
    //        }
    //    }
    //}
    //Document::doc_prop_value_strtype title;
    //doc.getProperty("Title", title);
    std::string title;
    doc.getString("Title", title);
    //std::string stitle = propstr_to_str(title);
    //brand.convertString(sbrand, UString::UTF_8);
    //std::cerr<<"[ABRAND]"<<sbrand<<std::endl;
    std::string sdocid;
    doc.getString("DOCID", sdocid);
    //bool debug = false;
    //if(sdocid=="418e05ac51300093a5175aa74d231f6f")
    //{
    //    debug = true;
    //}
    std::string spid;
    std::string old_spid;
    bool is_human_edit = false;
    {
        //boost::shared_lock<boost::shared_mutex> lock(mutex_);
        if(odb_->get(sdocid, spid)) 
        {
            //OfferDb::FlagType flag = 0;
            //odb_->get_flag(sdocid, flag);
            //if(flag==1)
            //{
            //    is_human_edit = true;
            //}
        }
    }
    old_spid = spid;
    //if(debug)
    //{
    //    std::cerr<<"[DEBUG]"<<sdocid<<","<<spid<<","<<old_spid<<std::endl;
    //}
    bool need_do_match = force_match? true:false;
    if(is_human_edit)
    {
        need_do_match = false;
    }
    else if(mode_>1)
    {
        need_do_match = true;
    }
    else if(spid.empty())
    {
        need_do_match = true;
    }
    else if(!title.empty()&&!category.empty())
    {
        need_do_match = true;
    }
    if(attr_!=NULL)
    {
        std::string sattr;
        doc.getString("Attribute", sattr);
        if(!sattr.empty())
        {
            try {
                sattr = attr_->attr_normalize(sattr);
            }
            catch(std::exception& ex)
            {
                sattr = "";
            }
            doc.property("Attribute") = str_to_propstr(sattr);
        }
    }
    Product product;
    if(need_do_match)
    {
        std::string oc;
        doc.getString("OriginalCategory", oc);
        //process maxent title
        if(maxent_title_!=NULL)
        {
            std::string source;
            doc.getString("Source", source);
            double price = ProductPrice::ParseDocPrice(doc);
            try{
                std::string mt = maxent_title_->predict(title, category, oc, (float)price, source);
                if(!mt.empty())
                {
                    doc.property("KNN") = str_to_propstr(mt);
                }
            }
            catch(std::exception& ex)
            {
            }
        }
        if(category.empty()&&!b5mm_.classifier_ip.empty())
        {
            msgpack::rpc::session se = sp_->get_session(b5mm_.classifier_ip, b5mm_.classifier_port);
            double price = ProductPrice::ParseDocPrice(doc);
            if(price<0.0) price = 0.0;
            try {
                std::string ecategory = se.call("classify",title, oc, price, 1, 1).get<std::string>();
                doc.property("Category") = str_to_propstr(ecategory);
            }
            catch(std::exception& ex)
            {
                //maybe server down, ignore;
            }
        }
        matcher_->Process(doc, product);
        if(product.type==Product::SPU&&!product.spid.empty())//spu matched
        {
            if(category.empty())
            {
                doc.property("Category") = str_to_propstr(product.scategory);
            }
        }
        else if(category.empty())
        {
            doc.property("Category") = str_to_propstr("");
        }
        status_.Insert(doc, product);
    }
    else
    {
        product.spid = spid;
        //matcher_->GetProduct(spid, product);
    }
    ProcessIUProduct_(doc, product, old_spid);
    //if(debug)
    //{
    //    spid.clear();
    //    doc.getString("uuid", spid);
    //    std::cerr<<"[DEBUG2]"<<sdocid<<","<<spid<<","<<old_spid<<std::endl;
    //}
}

void B5moProcessor::OMapperChange_(LastOMapperItem& item)
{
    boost::algorithm::trim(item.text);
    B5moSorter::Value value;
    Json::Reader json_reader;
    if(!value.Parse(item.text, &json_reader)) return;
    if(!OMap_(*(item.last_omapper), value.doc)) return;
    value.doc.type = UPDATE_SCD;
    ProcessIU_(value.doc, true);
    boost::unique_lock<boost::shared_mutex> lock(mutex_);
    item.writer->Append(value.doc, value.doc.type);
}

bool B5moProcessor::Generate(const std::string& mdb_instance, const std::string& last_mdb_instance)
{
    namespace bfs = boost::filesystem;
    if(!b5mm_.mobile_source.empty()&&bfs::exists(b5mm_.mobile_source))
    {
        LoadMobileSource(b5mm_.mobile_source);
    }
    if(!b5mm_.human_match.empty()&&bfs::exists(b5mm_.human_match))
    {
        SetHumanMatchFile(b5mm_.human_match);
    }
    const std::string& scd_path = b5mm_.scd_path;
    int thread_num = b5mm_.thread_num;
    //thread_num = 1;
    mode_ = b5mm_.mode;
    spumatch_count_ = 0;
    LOG(INFO)<<"B5moGenerator start, mode "<<mode_<<", thread_num "<<thread_num<<", scd "<<scd_path<<std::endl;
    if(mode_>0&&!b5mm_.rank_scd_path.empty())
    {
        LOG(INFO)<<"start to load rank scd"<<std::endl;
        std::string rscd;
        if(!B5mM::GetLatestScdPath(b5mm_.rank_scd_path, rscd))
        {
            LOG(INFO)<<"no rank scd available"<<std::endl;
        }
        else
        {
            std::size_t count = ScdParser::getScdDocCount(rscd);
            double prob = 1.0/count;
            rank_filter_ = new izenelib::util::BloomFilter<std::string>(count, prob);
            ScdDocProcessor::ProcessorType p = boost::bind(&B5moProcessor::RankProcess_, this, _1);
            ScdDocProcessor sd_processor(p, 1);
            sd_processor.AddInput(rscd);
            sd_processor.Process();
        }
    }
    if(!b5mm_.maxent_title_knowledge.empty())
    {
        maxent_title_ = new idmlib::knlp::Maxent_title(b5mm_.maxent_title_knowledge);
    }
    const std::string& knowledge = b5mm_.knowledge;
    matcher_ = new ProductMatcher;
    matcher_->SetCmaPath(b5mm_.cma_path);
    if(knowledge.empty()) matcher_->ForceOpen();
    else
    {
        if(!matcher_->Open(knowledge)) return false;
        if(!b5mm_.brand_ip.empty())
        {
            LOG(INFO)<<"start to init brand msgpack rpc at "<<b5mm_.brand_ip<<":"<<b5mm_.brand_port<<std::endl;
            try {
                boost::shared_ptr<msgpack::rpc::client> be(new msgpack::rpc::client(b5mm_.brand_ip, b5mm_.brand_port));
                matcher_->SetBrandExtractor(be);
            }
            catch(std::exception& ex)
            {
                LOG(ERROR)<<"brand init error "<<ex.what()<<std::endl;
                return false;
            }
        }
    }
    attr_ = matcher_->GetAttributeNormalize();
    std::string odb_path = B5MHelper::GetOdbPath(mdb_instance);
    if(bfs::exists(odb_path))
    {
        bfs::remove_all(odb_path);
    }
    std::string psm_path = B5MHelper::GetPsmPath(mdb_instance);
    if(bfs::exists(psm_path))
    {
        bfs::remove_all(psm_path);
    }
    if(!last_mdb_instance.empty())
    {
        std::string last_odb_path = B5MHelper::GetOdbPath(last_mdb_instance);
        if(!bfs::exists(last_odb_path))
        {
            LOG(ERROR)<<"last odb does not exist "<<last_odb_path<<std::endl;
            return false;
        }
        std::string last_psm_path = B5MHelper::GetPsmPath(last_mdb_instance);
        if(!bfs::exists(last_psm_path))
        {
            LOG(ERROR)<<"last psm does not exist "<<last_psm_path<<std::endl;
            return false;
        }
        LOG(INFO)<<"copying "<<last_odb_path<<" to "<<odb_path<<std::endl;
        izenelib::util::filesystem::recursive_copy_directory(last_odb_path, odb_path);
        LOG(INFO)<<"copy finished"<<std::endl;
        if(!b5mm_.rtype)
        {
            LOG(INFO)<<"copying "<<last_psm_path<<" to "<<psm_path<<std::endl;
            izenelib::util::filesystem::recursive_copy_directory(last_psm_path, psm_path);
            LOG(INFO)<<"copy finished"<<std::endl;
        }
    }
    odb_ = new OfferDb(odb_path);
    odb_->set_lazy_mode();
    if(!odb_->is_open())
    {
        LOG(INFO)<<"open odb..."<<std::endl;
        if(!odb_->open())
        {
            LOG(ERROR)<<"odb open fail"<<std::endl;
            return false;
        }
        LOG(INFO)<<"odb open successfully"<<std::endl;
    }
    if(!bfs::exists(psm_path))
    {
        boost::filesystem::create_directories(psm_path);
    }
    if(!knowledge.empty())
    {
        std::cerr<<"before open psm"<<std::endl;
        if(!matcher_->OpenPsm(psm_path))
        {
            LOG(ERROR)<<"open psm error"<<std::endl;
            return false;
        }
    }
    sp_ = new msgpack::rpc::session_pool();
    if(b5mm_.mode>0)
    {
        sp_->start(b5mm_.thread_num*2+2);
    }
    else
    {
        sp_->start(b5mm_.thread_num+2);
    }
    //if(!b5mm_.classifier_ip.empty())
    //{
    //    LOG(INFO)<<"start to init classifier msgpack rpc at "<<b5mm_.classifier_ip<<":"<<b5mm_.classifier_port<<std::endl;
    //    try {
    //        classifier_.reset(new msgpack::rpc::client(b5mm_.classifier_ip, b5mm_.classifier_port));
    //    }
    //    catch(std::exception& ex)
    //    {
    //        LOG(ERROR)<<"classifier init error "<<ex.what()<<std::endl;
    //        classifier_.reset();
    //    }
    //}
    //if(!b5mm_.comment_ip.empty())
    //{
    //    LOG(INFO)<<"start to init comment msgpack rpc at "<<b5mm_.comment_ip<<":"<<b5mm_.comment_port<<std::endl;
    //    try {
    //        comment_.reset(new msgpack::rpc::client(b5mm_.comment_ip, b5mm_.comment_port));
    //    }
    //    catch(std::exception& ex)
    //    {
    //        LOG(ERROR)<<"comment init error "<<ex.what()<<std::endl;
    //        comment_.reset();
    //        return false;
    //    }
    //}
    //std::string output_dir = B5MHelper::GetB5moPath(mdb_instance);
    std::string output_dir = b5mm_.b5mo_path;
    B5MHelper::PrepareEmptyDir(output_dir);
    std::string sorter_path = B5MHelper::GetB5moBlockPath(mdb_instance); 
    B5MHelper::PrepareEmptyDir(sorter_path);
    sorter_ = new B5moSorter(sorter_path, 200000);
    std::string omapper_path = B5MHelper::GetOMapperPath(mdb_instance);
    if(bfs::exists(omapper_path))
    {
        omapper_ = new OriginalMapper();
        LOG(INFO)<<"Opening omapper"<<std::endl;
        omapper_->Open(omapper_path);
        LOG(INFO)<<"Omapper opened"<<std::endl;
    }
    if(b5mm_.gen_tags&&!b5mm_.rtype)
    {
        if(b5mm_.tag_knowledge.empty())
        {
            LOG(ERROR)<<"tag knowledge does not specify"<<std::endl;
            return false;
        }
        tag_extractor_ = new ilplib::knlp::GetTags(b5mm_.tag_knowledge);
    }
    writer_.reset(new ScdTypeWriter(output_dir));
    ts_ = bfs::path(mdb_instance).filename().string();
    last_ts_="";
    if(!last_mdb_instance.empty())
    {
        last_ts_ = bfs::path(last_mdb_instance).filename().string();
        //std::string last_omapper_path = B5MHelper::GetOMapperPath(last_mdb_instance);
        //if(bfs::exists(last_omapper_path))
        //{
        //    OriginalMapper last_omapper;
        //    last_omapper.Open(last_omapper_path);
        //    if(bfs::exists(omapper_path))
        //    {
        //        last_omapper.Diff(omapper_path);
        //        if(last_omapper.Size()>0)
        //        {
        //            LOG(INFO)<<"Omapper changed, size "<<last_omapper.Size()<<std::endl;
        //            last_omapper.Show();
        //            std::string last_mirror = B5MHelper::GetB5moMirrorPath(last_mdb_instance);
        //            std::string last_block = last_mirror+"/block";
        //            std::ifstream ifs(last_block.c_str());
        //            std::string line;
        //            B5mThreadPool<LastOMapperItem> pool(thread_num, boost::bind(&B5moProcessor::OMapperChange_, this, _1));
        //            std::size_t p=0;
        //            while( getline(ifs, line))
        //            {
        //                ++p;
        //                if(p%100000==0)
        //                {
        //                    LOG(INFO)<<"Processing omapper change "<<p<<std::endl;
        //                }
        //                LastOMapperItem* item = new LastOMapperItem;
        //                item->last_omapper = &last_omapper;
        //                item->writer = writer_.get();
        //                item->text = line;
        //                pool.schedule(item);
        //                //boost::algorithm::trim(line);
        //                //B5moSorter::Value value;
        //                //if(!value.Parse(line, &json_reader)) continue;
        //                //if(!OMap_(last_omapper, value.doc)) continue;
        //                //value.doc.type = UPDATE_SCD;
        //                //ProcessIU_(value.doc, true);
        //                //writer->Append(value.doc, value.doc.type);
        //            }
        //            pool.wait();
        //            ifs.close();
        //            odb_->flush();
        //            LOG(INFO)<<"Omapper changing finished"<<std::endl;
        //        }
        //    }

        //}
    }

    //if(!matcher_->IsOpen())
    //{
        //LOG(INFO)<<"open matcher..."<<std::endl;
        //if(!matcher_->Open())
        //{
            //LOG(ERROR)<<"matcher open fail"<<std::endl;
            //return false;
        //}
    //}

    matcher_->SetMatcherOnly(true);

    std::string match_file = mdb_instance+"/match";
    std::string cmatch_file = mdb_instance+"/cmatch";
    match_ofs_.open(match_file.c_str());
    cmatch_ofs_.open(cmatch_file.c_str());
    //if(!human_match_file_.empty())
    //{
        //odb_->clear_all_flag();
        //std::ifstream ifs(human_match_file_.c_str());
        //std::string line;
        //uint32_t i=0;
        //while( getline(ifs, line))
        //{
            //boost::algorithm::trim(line);
            //++i;
            //if(i%100000==0)
            //{
                //LOG(INFO)<<"human match processing "<<i<<" item"<<std::endl;
            //}
            //boost::algorithm::trim(line);
            //std::vector<std::string> vec;
            //boost::algorithm::split(vec, line, boost::is_any_of(","));
            //if(vec.size()<2) continue;
            //const std::string& soid = vec[0];
            //const std::string& spid = vec[1];
            //std::string old_spid;
            //odb_->get(soid, old_spid);
            //if(spid!=old_spid)
            //{
                //changed_match_.insert(B5MHelper::StringToUint128(soid));
                //cmatch_ofs_<<soid<<","<<spid<<","<<old_spid<<std::endl;
                //odb_->insert(soid, spid);
            //}
            //odb_->insert_flag(soid, 1);
        //}
        //ifs.close();
        //odb_->flush();
    //}

    std::vector<std::string> r_scd_list;
    std::vector<std::string> u_scd_list;
    std::vector<std::string> d_scd_list;
    std::vector<std::string> scd_list;
    ScdParser::getScdList(scd_path, scd_list);
    for(std::size_t i=0;i<scd_list.size();i++)
    {
        const std::string& scd = scd_list[i];
        SCD_TYPE type = ScdParser::checkSCDType(scd);
        if(type==RTYPE_SCD)
        {
            r_scd_list.push_back(scd);
        }
        else if(type==DELETE_SCD)
        {
            d_scd_list.push_back(scd);
        }
        else
        {
            u_scd_list.push_back(scd);
        }
    }
    if(b5mm_.rtype)
    {
        if(!r_scd_list.empty()||!d_scd_list.empty())
        {
            ScdDocProcessor::ProcessorType p = boost::bind(&B5moProcessor::Process, this, _1);
            ScdDocProcessor sd_processor(p, 1);
            sd_processor.AddInput(r_scd_list);
            sd_processor.AddInput(d_scd_list);
            sd_processor.SetOutput(writer_);
            sd_processor.Process();
        }
    }
    else
    {
        if(!r_scd_list.empty()||!d_scd_list.empty())
        {
            ScdDocProcessor::ProcessorType p = boost::bind(&B5moProcessor::Process, this, _1);
            ScdDocProcessor sd_processor(p, 1);
            sd_processor.AddInput(r_scd_list);
            sd_processor.AddInput(d_scd_list);
            sd_processor.SetOutput(writer_);
            sd_processor.Process();
        }
        if(!u_scd_list.empty())
        {
            ScdDocProcessor::ProcessorType p = boost::bind(&B5moProcessor::Process, this, _1);
            ScdDocProcessor sd_processor(p, thread_num);
            //sd_processor.SetLimit(1000000ul);
            sd_processor.AddInput(u_scd_list);
            sd_processor.SetOutput(writer_);
            sd_processor.Process();
        }
    }
    matcher_->PendingFinish(boost::bind(&B5moProcessor::PendingProcess_, this, _1), thread_num);
    LOG(INFO)<<"start to close scd writer"<<std::endl;
    writer_->Close();
    LOG(INFO)<<"finished"<<std::endl;
    match_ofs_.close();
    cmatch_ofs_.close();
    LOG(INFO)<<"start to flush odb"<<std::endl;
    odb_->flush();
    LOG(INFO)<<"finished"<<std::endl;
    LOG(INFO)<<"Spu match count: "<<spumatch_count_<<std::endl;
    if(sorter_!=NULL)
    {
        sorter_->StageOne();
        delete sorter_;
        sorter_ = NULL;
    }
    if(omapper_!=NULL)
    {
        delete omapper_;
        omapper_ = NULL;
    }
    std::string status_path = mdb_instance+"/status";
    status_.Flush(status_path);
    sp_->end();
    return true;
}

bool B5moProcessor::OMap_(const OriginalMapper& omapper, Document& doc) const
{
    std::string source;
    std::string original_category;
    doc.getString("Source", source);
    doc.getString("OriginalCategory", original_category);
    if(source.empty()||original_category.empty()) return false;
    std::string category;
    if(omapper.Map(source, original_category, category))
    {
        doc.property("Category") = str_to_propstr(category);
        return true;
    }
    else
    {
        doc.property("Category") = str_to_propstr("");
        return false;
    }
}

void B5moProcessor::RankProcess_(ScdDocument& doc)
{
    std::string docid;
    doc.getString("DOCID", docid);
    if(docid.empty()) return;
    rank_filter_->Insert(docid);
}

