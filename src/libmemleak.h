#ifndef LIBMEMLEAK_H
#define LIBMEMLEAK_H


#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <new>
#include <unordered_map>
#include <vector>

struct allocation {
    void* ptr;
    int line;
    const char* file;
    std::size_t size;
    bool is_freed;
    bool is_array;
};

inline std::unordered_map<void*, allocation> allocations;
inline std::vector<allocation> freed_temp_zone;

inline thread_local bool tracking = false;
inline bool shutdown = false;
inline bool cleanup_registered = false;

void leakCheck() {
    shutdown = true;
    uint64_t bytes_leaked=0;
    if(allocations.size()>0) {
        for(auto &x : allocations) {
            if(!x.second.is_freed) {
                bytes_leaked += x.second.size;
                if(x.second.is_array) {
                    std::cerr << "Array that declared at file " <<
                    x.second.file << " at line " << x.second.line
                    << " is not freed\n";
                } else {
                    std::cerr << "Pointer that declared at file " <<
                    x.second.file << " at line " << x.second.line
                    << " is not freed\n";
                }
            }
        }
    }
    if(freed_temp_zone.size()>0) {
        for(auto &it : freed_temp_zone) {
            std::free(it.ptr);
        }
        freed_temp_zone.clear();
    }
    std::cerr << "Total bytes leaked: " << bytes_leaked << '\n';
    return;
}

void* operator new(std::size_t size, const char* file, int line) {
    if(!cleanup_registered) {
        atexit(leakCheck);
        cleanup_registered = true;
    }
    if(tracking) return std::malloc(size);
    tracking = true;
    void* ptr = std::malloc(size);
    if(ptr == nullptr) {
        throw std::bad_alloc();
    }
    allocations.insert_or_assign(ptr, allocation{
        .ptr=ptr,
        .line=line,
        .file=file,
        .size=size,
        .is_freed=false,
        .is_array = false
    });
    tracking = false;
    return ptr;
}

void* operator new[](std::size_t size, const char* file, int line) {
    if(allocations.size()==0) {
        atexit(leakCheck);
    }
    void* ptr = std::malloc(size);
    if(ptr == nullptr) {
        throw std::bad_alloc();
    }
    if(tracking) {
        return ptr;
    }
    tracking = true;
    allocations.insert_or_assign(ptr, allocation{
        .ptr=ptr,
        .line=line,
        .file=file,
        .size=size,
        .is_freed=false,
        .is_array=true
    });
    tracking = false;
    return ptr;
}

void operator delete(void* ptr) noexcept {
    if(shutdown) {
        std::free(ptr);
        return;
    }
    auto it = allocations.find(ptr);
    if(it==allocations.end()) {
        std::cerr << "Invalid pointer freeing\n";
        return;
    }
    else if (it->second.is_freed) {
        std::cerr << "Double free of pointer declared at " << it->second.file
        << " on line " << it->second.line << '\n';
        return;
    } else {
        freed_temp_zone.emplace_back(it->second);
        if(freed_temp_zone.size()>=64) {
            std::free(freed_temp_zone[0].ptr);
            freed_temp_zone.erase(freed_temp_zone.begin());
        }
        std::free(ptr);
        allocations.erase(it);
        return;
    }
}

void operator delete[](void* ptr) noexcept {
    if(shutdown) {
        std::free(ptr);
        return;
    }
    auto it = allocations.find(ptr);
    if(it==allocations.end()) {
        std::cerr << "Invalid pointer freeing\n";
        return;
    }
    else if (it->second.is_freed) {
        std::cerr << "Double free of pointer declared at " << it->second.file
        << " on line " << it->second.line << '\n';
        return;
    } else {
        freed_temp_zone.emplace_back(it->second);
        if(freed_temp_zone.size()>=64) {
            std::free(freed_temp_zone[0].ptr);
            freed_temp_zone.erase(freed_temp_zone.begin());
        }
        std::free(ptr);
        allocations.erase(it);
        return;
    }
}


#define new new(__FILE__, __LINE__)

#endif