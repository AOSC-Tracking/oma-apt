#include <apt-pkg/srcrecords.h>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <sstream>
#include <cstdint>

#include <assert.h>

#include <apt-pkg/configuration.h>
#include <apt-pkg/depcache.h>
#include <apt-pkg/sourcelist.h>
#include <apt-pkg/cachefile.h>
#include <apt-pkg/indexfile.h>
#include <apt-pkg/pkgcache.h>
#include <apt-pkg/pkgrecords.h>
#include <apt-pkg/version.h>
#include <apt-pkg/algorithms.h>
#include <apt-pkg/acquire.h>
#include <apt-pkg/acquire-item.h>
#include <apt-pkg/fileutl.h>

#include <apt-pkg/init.h>
#include <apt-pkg/pkgsystem.h>
#include <apt-pkg/policy.h>
#include <string>
// For Development Typing
#include "cxx-typing.h"
// Headers for the cxx bridge
#include "rust/cxx.h"
#include "rust-apt/src/raw.rs"
#include "apt-pkg.h"

// Couldn't get this to work without wrappers.
struct PCache {
	// Owned by us.
	pkgCacheFile *cache_file;

	// Borrowed from cache_file.
	pkgCache *cache;

	pkgSourceList *source;
};

struct PkgRecords {
	pkgRecords *records;

	pkgRecords::Parser *parser;

};

struct PkgIterator {
	pkgCache::PkgIterator iterator;
};

struct VerIterator {
	pkgCache::VerIterator iterator;
};

struct VerFileIterator {
	pkgCache::VerFileIterator iterator;
};

struct DescIterator {
	pkgCache::DescIterator iterator;
};

struct PkgFileIterator {
	pkgCache::PkgFileIterator iterator;
};

struct PkgIndexFile {
	// Owned by us.
	pkgIndexFile *index;
};

/// CXX Test Function
///
// int greet(rust::Str greetee) {
//   std::cout << "Hello, " << greetee << std::endl;
//   return get_num();
// }

/// Unused helper function since switching to CXX
// const char *to_c_string(std::string s) {
// 	char *cstr = new char[s.length()+1];
// 	std::strcpy(cstr, s.c_str());
// 	return cstr;
// }

/// Main Initializers for APT
///
void init_config_system() {
	pkgInitConfig(*_config);
	pkgInitSystem(*_config, _system);
}

PCache *pkg_cache_create() {
	pkgCacheFile *cache_file = new pkgCacheFile();
	PCache *ret = new PCache();
	cache_file->BuildSourceList();

	ret->cache_file = cache_file;
	ret->cache = cache_file->GetPkgCache();
	ret->source = cache_file->GetSourceList();
	return ret;
}

PkgRecords *pkg_records_create(PCache *pcache) {
	PkgRecords *records = new PkgRecords();
	records->records = new pkgRecords(*pcache->cache);
	// Can't populate the parser until we need it.
	records->parser = NULL;
	return records;
}

pkgDepCache *depcache_create(PCache *pcache) {
	pkgDepCache *depcache = pcache->cache_file->GetDepCache();
//	pkgApplyStatus(*depcache);
	return depcache;
}

void pkg_cache_release(PCache *cache) {
	// pkgCache and pkgDepCache are cleaned up with cache_file.
	delete cache->cache_file;
	delete cache;
}

void pkg_index_file_release(PkgIndexFile *wrapper) {
	delete wrapper;
}

void pkg_records_release(PkgRecords *records) {
	delete records -> records;
	delete records;
}

rust::Vec<SourceFile> source_uris(PCache *pcache) {
	pkgAcquire fetcher;
	rust::Vec<SourceFile> list;

	pcache->source->GetIndexes(&fetcher, true);
	pkgAcquire::UriIterator I = fetcher.UriBegin();

	for (; I != fetcher.UriEnd(); ++I) {
		list.push_back(
			SourceFile {
				I->URI,
				flNotDir(I->Owner->DestFile)
			}
		);
	}
	return list;
}

int32_t pkg_cache_compare_versions(PCache *cache, const char *left, const char *right) {
	// an int is returned here; presumably it will always be -1, 0 or 1.
	return cache->cache->VS->DoCmpVersion(left, left+strlen(left), right, right+strlen(right));
}

/// Basic Iterator Management
///
/// Iterator Creators
PkgIterator *pkg_begin(PCache *pcache) {
	PkgIterator *wrapper = new PkgIterator();
	wrapper->iterator = pcache->cache->PkgBegin();
	return wrapper;
}

PkgIterator *pkg_clone(PkgIterator *iterator) {
	PkgIterator *wrapper = new PkgIterator();
	wrapper->iterator = iterator->iterator;
	return wrapper;
}

VerIterator *ver_clone(VerIterator *iterator) {
	VerIterator *wrapper = new VerIterator();
	wrapper->iterator = iterator->iterator;
	return wrapper;
}

VerFileIterator *ver_file(VerIterator *wrapper) {
	VerFileIterator *new_wrapper = new VerFileIterator();
	new_wrapper->iterator = wrapper->iterator.FileList();
	return new_wrapper;
}

VerFileIterator *ver_file_clone(VerFileIterator *iterator) {
	VerFileIterator *wrapper = new VerFileIterator();
	wrapper->iterator = iterator->iterator;
	return wrapper;
}

VerIterator *pkg_current_version(PkgIterator *wrapper) {
	VerIterator *new_wrapper = new VerIterator();
	new_wrapper->iterator = wrapper->iterator.CurrentVer();
	return new_wrapper;
}

VerIterator *pkg_candidate_version(PCache *cache, PkgIterator *wrapper) {
	VerIterator *new_wrapper = new VerIterator();
	new_wrapper->iterator = cache->cache_file->GetPolicy()->GetCandidateVer(wrapper->iterator);
	return new_wrapper;
}

VerIterator *pkg_version_list(PkgIterator *wrapper) {
	VerIterator *new_wrapper = new VerIterator();
	new_wrapper->iterator = wrapper->iterator.VersionList();
	return new_wrapper;
}

PkgFileIterator *ver_pkg_file(VerFileIterator *wrapper) {
	PkgFileIterator *new_wrapper = new PkgFileIterator();
	new_wrapper->iterator = wrapper->iterator.File();
	return new_wrapper;
}

DescIterator *ver_desc_file(VerIterator *wrapper) {
	DescIterator *new_wrapper = new DescIterator();
	new_wrapper->iterator = wrapper->iterator.TranslatedDescription();
	return new_wrapper;
}

PkgIndexFile *pkg_index_file(PCache *pcache, PkgFileIterator *pkg_file) {
	PkgIndexFile *wrapper = new PkgIndexFile();
	pkgSourceList *SrcList = pcache->cache_file->GetSourceList();
	pkgIndexFile *Index;
	if (SrcList->FindIndex(pkg_file->iterator, Index) == false) { _system->FindIndex(pkg_file->iterator, Index);}
	wrapper->index = Index;
	return wrapper;
}

// These two are how we get a specific package by name.
PkgIterator *pkg_cache_find_name(PCache *pcache, rust::string name) {
	PkgIterator *wrapper = new PkgIterator();
	wrapper->iterator = pcache->cache->FindPkg(name.c_str());
	return wrapper;
}

PkgIterator *pkg_cache_find_name_arch(PCache *pcache, rust::string name, rust::string arch) {
	PkgIterator *wrapper = new PkgIterator();
	wrapper->iterator = pcache->cache->FindPkg(name.c_str(), arch.c_str());
	return wrapper;
}

/// Iterator Manipulation
///
void pkg_next(PkgIterator *wrapper) {
	++wrapper->iterator;
}

bool pkg_end(PkgIterator *wrapper) {
	return wrapper->iterator.end();
}

void pkg_release(PkgIterator *wrapper) {
	delete wrapper;
}

void ver_next(VerIterator *wrapper) {
	++wrapper->iterator;
}

bool ver_end(VerIterator *wrapper) {
	return wrapper->iterator.end();
}

void ver_release(VerIterator *wrapper) {
	// Maybe we should do this check no matter what?
	// if (wrapper->iterator == 0) { return; }
	delete wrapper;
}

void ver_file_next(VerFileIterator *wrapper) {
	++wrapper->iterator;
}

bool ver_file_end(VerFileIterator *wrapper) {
	return wrapper->iterator.end();
}

void ver_file_release(VerFileIterator *wrapper) {
	delete wrapper;
}

void pkg_file_release(PkgFileIterator *wrapper) {
	delete wrapper;
}

void ver_desc_release(DescIterator *wrapper) {
	delete wrapper;
}

/// Information Accessors
///
bool pkg_is_upgradable(pkgDepCache *depcache, PkgIterator *wrapper) {
	pkgCache::PkgIterator &pkg = wrapper->iterator;
	if (pkg.CurrentVer() == 0) { return false; }
	return (*depcache)[pkg].Upgradable();
}

bool pkg_is_auto_installed(pkgDepCache *depcache, PkgIterator *wrapper) {
	pkgCache::PkgIterator &pkg = wrapper->iterator;
	return (*depcache)[pkg].Flags & pkgCache::Flag::Auto;
}

bool pkg_is_garbage(pkgDepCache *depcache, PkgIterator *wrapper) {
	pkgCache::PkgIterator &pkg = wrapper->iterator;
	return (*depcache)[pkg].Garbage;
}

bool pkg_marked_install(pkgDepCache *depcache, PkgIterator *wrapper) {
	pkgCache::PkgIterator &pkg = wrapper->iterator;
	return (*depcache)[pkg].NewInstall();
}

bool pkg_marked_upgrade(pkgDepCache *depcache, PkgIterator *wrapper) {
	pkgCache::PkgIterator &pkg = wrapper->iterator;
	return (*depcache)[pkg].Upgrade();
}

bool pkg_marked_delete(pkgDepCache *depcache, PkgIterator *wrapper) {
	pkgCache::PkgIterator &pkg = wrapper->iterator;
	return (*depcache)[pkg].Delete();
}

bool pkg_marked_keep(pkgDepCache *depcache, PkgIterator *wrapper) {
	pkgCache::PkgIterator &pkg = wrapper->iterator;
	return (*depcache)[pkg].Keep();
}

bool pkg_marked_downgrade(pkgDepCache *depcache, PkgIterator *wrapper) {
	pkgCache::PkgIterator &pkg = wrapper->iterator;
	return (*depcache)[pkg].Downgrade();
}

bool pkg_marked_reinstall(pkgDepCache *depcache, PkgIterator *wrapper) {
	pkgCache::PkgIterator &pkg = wrapper->iterator;
	return (*depcache)[pkg].ReInstall();
}

bool pkg_is_now_broken(pkgDepCache *depcache, PkgIterator *wrapper) {
	pkgCache::PkgIterator &pkg = wrapper->iterator;
	return (*depcache)[pkg].NowBroken();
}

bool pkg_is_inst_broken(pkgDepCache *depcache, PkgIterator *wrapper) {
	pkgCache::PkgIterator &pkg = wrapper->iterator;
	return (*depcache)[pkg].InstBroken();
}

bool pkg_is_installed(PkgIterator *wrapper) {
	return !(wrapper->iterator.CurrentVer() == 0);
}

bool pkg_has_versions(PkgIterator *wrapper) {
	return wrapper->iterator.VersionList().end() == false;
}

bool pkg_has_provides(PkgIterator *wrapper) {
	return wrapper->iterator.ProvidesList().end() == false;
}

rust::string get_fullname(PkgIterator *wrapper, bool pretty) {
	return wrapper->iterator.FullName(pretty);
}

rust::string pkg_name(PkgIterator *wrapper) {
	return wrapper->iterator.Name();
}

rust::string pkg_arch(PkgIterator *wrapper) {
	return wrapper->iterator.Arch();
}

int32_t pkg_id(PkgIterator *wrapper) {
	return wrapper->iterator->ID;
}

int32_t pkg_current_state(PkgIterator *wrapper) {
	return wrapper->iterator->CurrentState;
}

int32_t pkg_inst_state(PkgIterator *wrapper) {
	return wrapper->iterator->InstState;
}

int32_t pkg_selected_state(PkgIterator *wrapper) {
	return wrapper->iterator->SelectedState;
}

bool pkg_essential(PkgIterator *wrapper) {
	return (wrapper->iterator->Flags & pkgCache::Flag::Essential) != 0;
}

rust::string ver_arch(VerIterator *wrapper) {
	return wrapper->iterator.Arch();
}

rust::string ver_str(VerIterator *wrapper) {
	return wrapper->iterator.VerStr();
}

rust::string ver_section(VerIterator *wrapper) {
   return wrapper->iterator.Section();
}

rust::string ver_priority_str(VerIterator *wrapper) {
	return wrapper->iterator.PriorityType();
}

rust::string ver_source_package(VerIterator *wrapper) {
	return wrapper->iterator.SourcePkgName();
}

rust::string ver_source_version(VerIterator *wrapper) {
	return wrapper->iterator.SourceVerStr();
}

rust::string ver_name(VerIterator *wrapper) {
	return wrapper->iterator.ParentPkg().Name();
}

int32_t ver_size(VerIterator *wrapper) {
	return wrapper->iterator->Size;
}

int32_t ver_installed_size(VerIterator *wrapper) {
	return wrapper->iterator->InstalledSize;
}

bool ver_downloadable(VerIterator *wrapper) {
	return wrapper->iterator.Downloadable();
}

int32_t ver_id(VerIterator *wrapper) {
	return wrapper->iterator->ID;
}

bool ver_installed(VerIterator *wrapper) {
	return wrapper->iterator.ParentPkg().CurrentVer() == wrapper->iterator;
}

int32_t ver_priority(PCache *pcache, VerIterator *wrapper) {
	pkgCache::VerIterator &ver = wrapper->iterator;
	return pcache->cache_file->GetPolicy()->GetPriority(ver);
}

/// Package Record Management
///
// Moves the Records into the correct place
void ver_file_lookup(PkgRecords *records, VerFileIterator *wrapper) {
	records->parser = &records->records->Lookup(wrapper->iterator);
}

void desc_file_lookup(PkgRecords *records, DescIterator *wrapper) {
	records->parser = &records->records->Lookup(wrapper->iterator.FileList());
}

rust::string ver_uri(PkgRecords *records, PkgIndexFile *index_file) {
	return index_file->index->ArchiveURI(records->parser->FileName());
}

rust::string long_desc(PkgRecords *records) {
	return records->parser->LongDesc();
}

rust::string short_desc(PkgRecords *records) {
	return records->parser->ShortDesc();
}

rust::string hash_find(PkgRecords *records, rust::string hash_type) {
	auto hashes = records->parser->Hashes();
	auto hash = hashes.find(hash_type.c_str());
	if (hash == NULL) { return "KeyError"; }
	return hash->HashValue();
}

// #define VALIDATE_ITERATOR(I) {
// 	if ((I).Cache() != &depcache->GetCache()) return(false);
// 	return(true); }

// template<typename Iterator>
// static bool _validate(Iterator iter, pkgDepCache *depcache) {
// 	if (iter.Cache() != &depcache->GetCache())
// 	{return false;} else {return true;}
// }

// bool validate(VerIterator *wrapper, PCache *pcache) {
// 	// if (wrapper->iterator.Cache() != &pcache->depcache->GetCache())
// 	// {return false;} else {return true;}
// 	return _validate(wrapper->iterator, pcache->depcache);
// }

// bool validate(VerIterator *wrapper, PCache *pcache) {
// 	if (wrapper->iterator.Cache() != &pcache->depcache->GetCache())
// 	{return false;} else {return true;}
// }

// PDepIterator *ver_iter_dep_iter(VerIterator *wrapper) {
// 	PDepIterator *new_wrapper = new PDepIterator();
// 	new_wrapper->iterator = wrapper->iterator.DependsList();
// //	new_wrapper->cache = wrapper->cache;
// 	return new_wrapper;
// }

// void dep_iter_release(PDepIterator *wrapper) {
// 	delete wrapper;
// }

// void dep_iter_next(PDepIterator *wrapper) {
// 	++wrapper->iterator;
// }

// bool dep_iter_end(PDepIterator *wrapper) {
// 	return wrapper->iterator.end();
// }

// PkgIterator *dep_iter_target_pkg(PDepIterator *wrapper) {
// 	PkgIterator *new_wrapper = new PkgIterator();
// 	new_wrapper->iterator = wrapper->iterator.TargetPkg();
// //	new_wrapper->cache = wrapper->cache;
// 	return new_wrapper;
// }

// const char *dep_iter_target_ver(PDepIterator *wrapper) {
// 	return wrapper->iterator.TargetVer();
// }

// const char *dep_iter_comp_type(PDepIterator *wrapper) {
// 	return wrapper->iterator.CompType();
// }

// const char *dep_iter_dep_type(PDepIterator *wrapper) {
// 	return wrapper->iterator.DepType();
// }


// Look at ver_uri for answers.
// Maybe *get_hash(idk, *for_real) -> hash {
	// std::cout << "SHA256 = ";
	// auto hashes = parser->Hashes();
	// auto hash = hashes.find("sha256");
	// if (hash == NULL) {std::cout << "NULL";} else {std::cout << parser->Hashes().find("sha256")->HashValue();}
// }

// const char *ver_file_parser_maintainer(VerFileParser *parser) {
// 	std::string maint = parser->parser->Maintainer();
// 	return to_c_string(maint);
// }

// const char *ver_file_parser_homepage(VerFileParser *parser) {
// 	std::string hp = parser->parser->Homepage();
// 	return to_c_string(hp);
// }


/// Unused Functions
/// They may be used in the future
///
// void pkg_file_iter_next(PkgFileIterator *wrapper) {
// 	++wrapper->iterator;
// }

// bool pkg_file_iter_end(PkgFileIterator *wrapper) {
// 	return wrapper->iterator.end();
// }

// const char *pkg_file_iter_file_name(PkgFileIterator *wrapper) {
// 	return wrapper->iterator.FileName();
// }

// const char *pkg_file_iter_archive(PkgFileIterator *wrapper) {
// 	return wrapper->iterator.Archive();
// }

// const char *pkg_file_iter_version(PkgFileIterator *wrapper) {
// 	return wrapper->iterator.Version();
// }

// const char *pkg_file_iter_origin(PkgFileIterator *wrapper) {
// 	return wrapper->iterator.Origin();
// }

// const char *pkg_file_iter_codename(PkgFileIterator *wrapper) {
// 	return wrapper->iterator.Codename();
// }

// const char *pkg_file_iter_label(PkgFileIterator *wrapper) {
// 	return wrapper->iterator.Label();
// }

// const char *pkg_file_iter_site(PkgFileIterator *wrapper) {
// 	return wrapper->iterator.Site();
// }

// const char *pkg_file_iter_component(PkgFileIterator *wrapper) {
// 	return wrapper->iterator.Component();
// }

// const char *pkg_file_iter_architecture(PkgFileIterator *wrapper) {
// 	return wrapper->iterator.Architecture();
// }

// const char *pkg_file_iter_index_type(PkgFileIterator *wrapper) {
// 	return wrapper->iterator.IndexType();
// }
