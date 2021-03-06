#include <idmlib/semantic_space/random_indexing_generator.h>
#include <am/matrix/matrix_mem_io.h>
#include <am/matrix/matrix_file_io.h>
#include <boost/regex.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

int main(int argc, char** argv)
{
  //use sparse vector as container
  typedef idmlib::sim::TermSimilarity<izenelib::am::MatrixFileVLIo, izenelib::am::SparseVector<int64_t,uint16_t> > TermSimilarityType;
  if(argc!=3)
  {
    std::cerr<<"Usage: ./kp_similarity <inverted_text_file> <output_file>"<<std::endl;
    return -1;
  }
  std::string kp_inverted_file(argv[1]);
  std::string output_file(argv[2]);
  std::cout<<"file : "<<kp_inverted_file<<std::endl;
  std::cout<<"output file : "<<output_file<<std::endl;
  std::ifstream ifs(kp_inverted_file.c_str());
  std::string line;
  std::string test_dir = "./term_sim_test";
  boost::filesystem::remove_all(test_dir);
  uint8_t sim_per_term = 5;
  typedef TermSimilarityType::SimTableType SimTableType;
  SimTableType* table = new SimTableType(test_dir+"/table", sim_per_term);
  TermSimilarityType* sim = new TermSimilarityType(test_dir, table, sim_per_term, 0.4);
  if(!sim->Open())
  {
    std::cout<<"open error"<<std::endl;
    return -1;
  }
  std::vector<std::string> kp_list;
  uint32_t line_num = 0;
  while( getline(ifs, line) )
  {
    line_num++;
    if(line_num%1==0)
    {
      std::cout<<"Processing line "<<line_num<<std::endl;
    } 
    std::vector<std::string> vec1;
    boost::algorithm::split( vec1, line, boost::algorithm::is_any_of("\t") );
    if(vec1.size()!=2)
    {
      std::cout<<"invalid line: "<<line<<std::endl;
      continue;
    }
    std::string kp = vec1[0];
    std::vector<std::string> vec2;
    boost::algorithm::split( vec2, vec1[1], boost::algorithm::is_any_of("|") );
    std::vector<std::pair<uint32_t, uint32_t> > doc_item_list;
    for(uint32_t i=0;i<vec2.size();i++)
    {
      std::vector<std::string> doc_item;
      boost::algorithm::split( doc_item, vec2[i], boost::algorithm::is_any_of(",") );
      if( doc_item.size()!=2 )
      {
        std::cout<<"invalid doc item: "<<vec2[i]<<std::endl;
        continue;
      }
      uint32_t docid = boost::lexical_cast<uint32_t>(doc_item[0]);
      uint32_t freq = boost::lexical_cast<uint32_t>(doc_item[1]);
      doc_item_list.push_back(std::make_pair(docid, freq));
    }
    if(doc_item_list.empty())
    {
      std::cout<<"invalid line: "<<line<<std::endl;
      continue;
    }
    uint32_t kp_id = kp_list.size()+1;
    //append this kp term
    if(!sim->Append(kp_id, doc_item_list))
    {
      std::cout<<"append error"<<std::endl;
      return -1;
    }
    kp_list.push_back(kp);
  }
  std::cout<<"Start computing similarity"<<std::endl;
  if(!sim->Compute())
  {
    std::cout<<"compute error"<<std::endl;
    return -1;
  }
  std::cout<<"Computing finished."<<std::endl;
  std::ofstream ofs(output_file.c_str());
  for(uint32_t i=0;i<kp_list.size();i++)
  {
    uint32_t kp_id = i+1;
    std::vector<uint32_t> sim_list;
    if(!sim->GetSimVector(kp_id, sim_list))
    {
      std::cout<<"no sim list for kp id: "<<kp_id<<std::endl;
    }
    else
    {
      std::string kp = kp_list[kp_id-1];
      ofs<<kp<<" : ";
      for(uint32_t j=0;j<sim_list.size();j++)
      {
        std::string sim_kp = kp_list[sim_list[j]-1];
        ofs<<sim_kp<<",";
      }
      ofs<<std::endl;
    }
  }
  ofs.close();
  std::cout<<"Output finished"<<std::endl;
  delete sim;
  delete table;
  return 0;
}
