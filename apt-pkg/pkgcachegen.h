// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
/* ######################################################################
   
   Package Cache Generator - Generator for the cache structure.
   
   This builds the cache structure from the abstract package list parser. 
   Each archive source has it's own list parser that is instantiated by
   the caller to provide data for the generator. 
   
   Parts of the cache are created by this generator class while other
   parts are created by the list parser. The list parser is responsible
   for creating version, depends and provides structures, and some of
   their contents
   
   ##################################################################### */
									/*}}}*/
#ifndef PKGLIB_PKGCACHEGEN_H
#define PKGLIB_PKGCACHEGEN_H

#include <apt-pkg/md5.h>
#include <apt-pkg/mmap.h>
#include <apt-pkg/pkgcache.h>
#include <apt-pkg/cacheiterators.h>
#include <apt-pkg/macros.h>

#include <vector>
#include <string>
#if __cplusplus >= 201103L
#include <unordered_map>
#endif
#ifdef APT_PKG_EXPOSE_STRING_VIEW
#include <apt-pkg/string_view.h>
#endif

class FileFd;
class pkgSourceList;
class OpProgress;
class pkgIndexFile;
class pkgCacheListParser;

class APT_HIDDEN pkgCacheGenerator					/*{{{*/
{
   APT_HIDDEN map_stringitem_t WriteStringInMap(std::string const &String) { return WriteStringInMap(String.c_str()); };
   APT_HIDDEN map_stringitem_t WriteStringInMap(const char *String);
   APT_HIDDEN map_stringitem_t WriteStringInMap(const char *String, const unsigned long &Len);
   APT_HIDDEN map_pointer_t AllocateInMap(const unsigned long &size);

   // Dirty hack for public users that do not use C++11 yet
#if __cplusplus >= 201103L
   std::unordered_map<std::string,map_stringitem_t> strMixed;
   std::unordered_map<std::string,map_stringitem_t> strSections;
   std::unordered_map<std::string,map_stringitem_t> strPkgNames;
   std::unordered_map<std::string,map_stringitem_t> strVersions;
#endif

   friend class pkgCacheListParser;
   typedef pkgCacheListParser ListParser;

   public:

   template<typename Iter> class Dynamic {
      public:
      static std::vector<Iter*> toReMap;
      explicit Dynamic(Iter &I) {
	 toReMap.push_back(&I);
      }

      ~Dynamic() {
	 toReMap.pop_back();
      }
   };

   protected:

   DynamicMMap &Map;
   pkgCache Cache;
   OpProgress *Progress;

   std::string RlsFileName;
   pkgCache::ReleaseFile *CurrentRlsFile;
   std::string PkgFileName;
   pkgCache::PackageFile *CurrentFile;

#ifdef APT_PKG_EXPOSE_STRING_VIEW
   bool NewGroup(pkgCache::GrpIterator &Grp, APT::StringView Name);
   bool NewPackage(pkgCache::PkgIterator &Pkg, APT::StringView Name, APT::StringView Arch);
#endif
   bool NewFileVer(pkgCache::VerIterator &Ver,ListParser &List);
   bool NewFileDesc(pkgCache::DescIterator &Desc,ListParser &List);
   bool NewDepends(pkgCache::PkgIterator &Pkg, pkgCache::VerIterator &Ver,
		   map_pointer_t const Version, uint8_t const Op,
		   uint8_t const Type, map_pointer_t* &OldDepLast);
   map_pointer_t NewVersion(pkgCache::VerIterator &Ver,const std::string &VerStr,map_pointer_t const Next) APT_DEPRECATED
   { return NewVersion(Ver, VerStr, 0, 0, Next); }
   map_pointer_t NewVersion(pkgCache::VerIterator &Ver,const std::string &VerStr,
			    map_pointer_t const ParentPkg, unsigned short const Hash,
			    map_pointer_t const Next);
   map_pointer_t NewDescription(pkgCache::DescIterator &Desc,const std::string &Lang,const MD5SumValue &md5sum,map_stringitem_t const idxmd5str);
   bool NewProvides(pkgCache::VerIterator &Ver, pkgCache::PkgIterator &Pkg,
		    map_stringitem_t const ProvidesVersion, uint8_t const Flags);

   public:

   enum StringType { MIXED, PKGNAME, VERSIONNUMBER, SECTION };
   map_stringitem_t StoreString(StringType const type, const char * S, unsigned int const Size);

#ifdef APT_PKG_EXPOSE_STRING_VIEW
   inline map_stringitem_t StoreString(enum StringType const type, APT::StringView S) {return StoreString(type, S.data(),S.length());};
#endif

   void DropProgress() {Progress = 0;};
   bool SelectFile(const std::string &File,pkgIndexFile const &Index, std::string const &Architecture, std::string const &Component, unsigned long Flags = 0);
   bool SelectReleaseFile(const std::string &File, const std::string &Site, unsigned long Flags = 0);
   bool MergeList(ListParser &List,pkgCache::VerIterator *Ver = 0);
   inline pkgCache &GetCache() {return Cache;};
   inline pkgCache::PkgFileIterator GetCurFile()
         {return pkgCache::PkgFileIterator(Cache,CurrentFile);};
   inline pkgCache::RlsFileIterator GetCurRlsFile()
         {return pkgCache::RlsFileIterator(Cache,CurrentRlsFile);};

   APT_PUBLIC static bool MakeStatusCache(pkgSourceList &List,OpProgress *Progress,
			MMap **OutMap = 0,bool AllowMem = false);
   APT_HIDDEN static bool MakeStatusCache(pkgSourceList &List,OpProgress *Progress,
			MMap **OutMap,pkgCache **OutCache, bool AllowMem = false);
   APT_PUBLIC static bool MakeOnlyStatusCache(OpProgress *Progress,DynamicMMap **OutMap);

   void ReMap(void const * const oldMap, void const * const newMap);

   pkgCacheGenerator(DynamicMMap *Map,OpProgress *Progress);
   virtual ~pkgCacheGenerator();

   private:
   void * const d;
   APT_HIDDEN bool MergeListGroup(ListParser &List, std::string const &GrpName);
   APT_HIDDEN bool MergeListPackage(ListParser &List, pkgCache::PkgIterator &Pkg);
   APT_HIDDEN bool MergeListVersion(ListParser &List, pkgCache::PkgIterator &Pkg,
			 std::string const &Version, pkgCache::VerIterator* &OutVer);

   APT_HIDDEN bool AddImplicitDepends(pkgCache::GrpIterator &G, pkgCache::PkgIterator &P,
			   pkgCache::VerIterator &V);
   APT_HIDDEN bool AddImplicitDepends(pkgCache::VerIterator &V, pkgCache::PkgIterator &D);

   APT_HIDDEN bool AddNewDescription(ListParser &List, pkgCache::VerIterator &Ver,
	 std::string const &lang, MD5SumValue const &CurMd5, map_stringitem_t &md5idx);
};
									/*}}}*/
// This is the abstract package list parser class.			/*{{{*/
class APT_HIDDEN pkgCacheListParser
{
   pkgCacheGenerator *Owner;
   friend class pkgCacheGenerator;

   // Some cache items
   pkgCache::VerIterator OldDepVer;
   map_pointer_t *OldDepLast;

   void * const d;

   protected:

   inline map_stringitem_t StoreString(pkgCacheGenerator::StringType const type, const char *S,unsigned int Size) {return Owner->StoreString(type, S, Size);};
#ifdef APT_PKG_EXPOSE_STRING_VIEW
   inline map_stringitem_t StoreString(pkgCacheGenerator::StringType const type, APT::StringView S) {return Owner->StoreString(type, S);};
#endif

   inline map_stringitem_t WriteString(const std::string &S) {return Owner->WriteStringInMap(S);};
   inline map_stringitem_t WriteString(const char *S,unsigned int Size) {return Owner->WriteStringInMap(S,Size);};
#ifdef APT_PKG_EXPOSE_STRING_VIEW
   bool NewDepends(pkgCache::VerIterator &Ver,APT::StringView Package, APT::StringView Arch,
		   APT::StringView Version,uint8_t const Op,
		   uint8_t const Type);
   bool NewProvides(pkgCache::VerIterator &Ver,APT::StringView PkgName,
		    APT::StringView PkgArch, APT::StringView Version,
		    uint8_t const Flags);
   bool NewProvidesAllArch(pkgCache::VerIterator &Ver, APT::StringView Package,
			   APT::StringView Version, uint8_t const Flags);
#endif
   public:
   
   // These all operate against the current section
   virtual std::string Package() = 0;
   virtual std::string Architecture() = 0;
   virtual bool ArchitectureAll() = 0;
   virtual std::string Version() = 0;
   virtual bool NewVersion(pkgCache::VerIterator &Ver) = 0;
   virtual std::string Description(std::string const &lang) = 0;
   virtual std::vector<std::string> AvailableDescriptionLanguages() = 0;
   virtual MD5SumValue Description_md5() = 0;
   virtual unsigned short VersionHash() = 0;
   /** compare currently parsed version with given version
    *
    * \param Hash of the currently parsed version
    * \param Ver to compare with
    */
   virtual bool SameVersion(unsigned short const Hash, pkgCache::VerIterator const &Ver);
   virtual bool UsePackage(pkgCache::PkgIterator &Pkg,
			   pkgCache::VerIterator &Ver) = 0;
   virtual map_filesize_t Offset() = 0;
   virtual map_filesize_t Size() = 0;
   
   virtual bool Step() = 0;
   
   virtual bool CollectFileProvides(pkgCache &/*Cache*/,
				    pkgCache::VerIterator &/*Ver*/) {return true;};

   pkgCacheListParser();
   virtual ~pkgCacheListParser();
};
									/*}}}*/

APT_DEPRECATED_MSG("Use pkgCacheGenerator::MakeStatusCache instead") bool pkgMakeStatusCache(pkgSourceList &List,OpProgress &Progress,
			MMap **OutMap = 0,bool AllowMem = false);
APT_DEPRECATED_MSG("Use pkgCacheGenerator::MakeOnlyStatusCache instead") bool pkgMakeOnlyStatusCache(OpProgress &Progress,DynamicMMap **OutMap);

#endif
