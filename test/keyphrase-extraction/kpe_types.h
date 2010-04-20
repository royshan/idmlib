#include <ir/id_manager/IDManager.h>
#include <string>
#include <boost/noncopyable.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/filesystem.hpp>
class TestIDManager : public boost::noncopyable
{
    public:
        TestIDManager(const std::string& path):path_(path)
        /*:strStorage_(path+"/id_ustring_map")*/
        {
            strStorage_=new izenelib::ir::idmanager::HDBIDStorage< wiselib::UString, uint32_t>(path+"/id_ustring_map");
        }
  
        ~TestIDManager()
        {
            if(strStorage_)
                delete strStorage_;
        }
        
        bool getTermIdByTermString(const wiselib::UString& ustr, uint32_t& termId)
        {
            termId = izenelib::util::HashFunction<wiselib::UString>::generateHash32(ustr);
            boost::mutex::scoped_lock lock(mutex_);
            strStorage_->put(termId, ustr);
            return true;
        }
        
        bool getTermIdByTermString(const wiselib::UString& ustr, char pos, uint32_t& termId)
        {
            termId = izenelib::util::HashFunction<wiselib::UString>::generateHash32(ustr);
            boost::mutex::scoped_lock lock(mutex_);
            strStorage_->put(termId, ustr);
            return true;
        }
        
        bool getTermStringByTermId(uint32_t termId, wiselib::UString& ustr)
        {
            boost::mutex::scoped_lock lock(mutex_);
            return strStorage_->get(termId, ustr);
        }
        
        void put(uint32_t termId, const wiselib::UString& ustr)
        {
            boost::mutex::scoped_lock lock(mutex_);
            strStorage_->put(termId, ustr);
            
        }
        
        bool isKP(uint32_t termId)
        {
            return false;
        }
        
        void getAnalysisTermIdList(const wiselib::UString& str, std::vector<uint32_t>& termIdList)
        {
            
        }
        
        void getAnalysisTermIdList(const wiselib::UString& str, std::vector<wiselib::UString>& termList, std::vector<uint32_t>& idList, std::vector<char>& posInfoList, std::vector<uint32_t>& positionList)
        {
            
        }
        
        void flush()
        {
            strStorage_->flush();
        }

        void close()
        {
            strStorage_->close();
        }
        
    private:        
        izenelib::ir::idmanager::HDBIDStorage< wiselib::UString, uint32_t>* strStorage_;
        boost::mutex mutex_;
        std::string path_;
        
};
