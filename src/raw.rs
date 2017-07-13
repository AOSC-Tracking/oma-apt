/// In general:
///  * `*mut c_void` are to be released by the appropriate function
///  * `*const c_chars` are short-term borrows
///  * `*mut c_chars` are to be freed by `libc::free`.

use libc::c_void;
use libc::c_char;

pub type PCache = *mut c_void;
pub type PPkgIterator = *mut c_void;
pub type PVerIterator = *mut c_void;

#[link(name = "apt-pkg")]
extern "C" {
    /// Must be called exactly once, before anything else?
    fn init_config_system();
    fn pkg_cache_create() -> PCache;


    // Package iterators
    // =================

    pub fn pkg_cache_pkg_iter(cache: PCache) -> PPkgIterator;
    pub fn pkg_cache_find_name(cache: PCache, name: *const c_char) -> PPkgIterator;
    pub fn pkg_cache_find_name_arch(
        cache: PCache,
        name: *const c_char,
        arch: *const c_char,
    ) -> PPkgIterator;
    pub fn pkg_iter_release(iterator: PPkgIterator);

    pub fn pkg_iter_next(iterator: PPkgIterator);
    pub fn pkg_iter_end(iterator: PPkgIterator) -> bool;


    // Package iterator accessors
    // ==========================

    pub fn pkg_iter_name(iterator: PPkgIterator) -> *const c_char;
    pub fn pkg_iter_arch(iterator: PPkgIterator) -> *const c_char;
    pub fn pkg_iter_current_version(iterator: PPkgIterator) -> *const c_char;
    pub fn pkg_iter_candidate_version(iterator: PPkgIterator) -> *const c_char;
    pub fn pkg_iter_pretty(cache: PCache, iterator: PPkgIterator) -> *mut c_char;


    // Version iterators
    // =================

    pub fn pkg_iter_ver_iter(pkg: PPkgIterator) -> PVerIterator;
    pub fn ver_iter_release(iterator: PVerIterator);

    pub fn ver_iter_next(iterator: PVerIterator);
    pub fn ver_iter_end(iterator: PVerIterator) -> bool;

    // Version accessors
    // =================

    pub fn ver_iter_version(iterator: PVerIterator) -> *mut c_char;
    pub fn ver_iter_section(iterator: PVerIterator) -> *mut c_char;
    pub fn ver_iter_source_package(iterator: PVerIterator) -> *mut c_char;
    pub fn ver_iter_source_version(iterator: PVerIterator) -> *mut c_char;
    pub fn ver_iter_arch(iterator: PVerIterator) -> *mut c_char;
    pub fn ver_iter_priority(iterator: PVerIterator) -> i32;
}

pub fn pkg_cache_get() -> PCache {
    CACHE.ptr
}

struct CacheHolder {
    ptr: PCache,
}

unsafe impl Sync for CacheHolder {}

lazy_static! {
    static ref CACHE: CacheHolder = {
        unsafe {
            init_config_system();
            CacheHolder {
                ptr: pkg_cache_create()
            }
        }
    };
}
