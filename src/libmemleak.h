#ifndef LIBMEMLEAK_H
#define LIBMEMLEAK_H


#include <algorithm>
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

struct NoopHasher {
    std::size_t operator()(const void* x) const noexcept {
        std::uintptr_t address = reinterpret_cast<std::uintptr_t>(x);
        return address>>4;
    }
};

inline std::unordered_map<void*, allocation, NoopHasher> allocations;
inline std::vector<allocation> freed_temp_zone;

inline thread_local bool tracking = false;
inline bool shutdown = false;
inline bool cleanup_registered = false;
inline uint32_t total_allocations = 0;



void leakCheck() {
    if(shutdown) return;
    shutdown = true;
    tracking = true;
    uint64_t bytes_leaked=0;
    if(!allocations.empty()) {
        auto it = allocations.begin();
        while(it!=allocations.end()) {
            if(!it->second.is_freed) {
                bytes_leaked += it->second.size;
                if(it->second.is_array) {
                    std::cerr << "Array that declared at file " <<
                    it->second.file << " at line " << it->second.line
                    << " is not freed\n";
                } else {
                    std::cerr << "Pointer that declared at file " <<
                    it->second.file << " at line " << it->second.line
                    << " is not freed\n";
                }
                it = allocations.erase(it);
            } else ++it;
        }
    }
    allocations.clear();
    freed_temp_zone.clear();
    std::cerr << "Total bytes leaked: " << bytes_leaked << '\n';
    std::cerr << "Total amount of allocations: " << total_allocations << '\n';
    return;
}

void* operator new(std::size_t size, const char* file, int line) {
    if(tracking||shutdown) return std::malloc(size);
    if(!cleanup_registered) {
        atexit(leakCheck);
        cleanup_registered = true;
    }
    total_allocations++;
    tracking = true;
    void* ptr = std::malloc(size);
    if(ptr == nullptr) {
        tracking = false;
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
    if(tracking||shutdown) return std::malloc(size);
    if(!cleanup_registered) {
        atexit(leakCheck);
        cleanup_registered = true;
    }
    total_allocations++;
    tracking = true;
    void* ptr = std::malloc(size);
    if(ptr == nullptr) {
        tracking = false;
        throw std::bad_alloc();
    }
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
    if(ptr==nullptr) return;
    if(shutdown) {
        std::free(ptr);
        return;
    }
    if(tracking) {
        std::free(ptr);
        return;
    }
    tracking = true;
    auto it = allocations.find(ptr);
    if(it==allocations.end()) {
        for(auto &x : freed_temp_zone) {
            if(x.ptr==ptr) {
                std::cerr << "Double free of pointer declared at " << x.file
                << " on line " << x.line << '\n';
                tracking = false;
                return;
            }
        }
        std::cerr << "Invalid pointer freeing\n";
        tracking = false;
        return;
    }
    else if (it->second.is_freed) {
        std::cerr << "Double free of pointer declared at " << it->second.file
        << " on line " << it->second.line << '\n';
        tracking = false;
        return;
    } else {
        if(it->second.is_array) {
            std::cerr << "Mismatched free(use 'delete[]' instead) of pointer declared at " 
            << it->second.file
            << " on line " << it->second.line << '\n';
        }
        it->second.is_freed = true;
        freed_temp_zone.emplace_back(it->second);
        if(freed_temp_zone.size()>=64) {
            freed_temp_zone.erase(freed_temp_zone.begin());
        }
        std::free(ptr);
        allocations.erase(it);
        tracking = false;
        return;
    }
}

void operator delete[](void* ptr) noexcept {
    if(ptr==nullptr) return;
    if(shutdown) {
        std::free(ptr);
        return;
    }
    if(tracking) {
        std::free(ptr);
        return;
    }
    tracking = true;
    auto it = allocations.find(ptr);
    if(it==allocations.end()) {
        for(auto &x : freed_temp_zone) {
            if(x.ptr==ptr) {
                std::cerr << "Double free of pointer declared at " << x.file
                << " on line " << x.line << '\n';
                tracking = false;
                return;
            }
        }
        std::cerr << "Invalid pointer freeing\n";
        tracking = false;
        return;
    }
    else if (it->second.is_freed) {
        std::cerr << "Double free of pointer declared at " << it->second.file
        << " on line " << it->second.line << '\n';
        tracking = false;
        return;
    } else {
        if(!it->second.is_array) {
            std::cerr << "Mismatched free(use 'delete' instead) of pointer declared at " 
            << it->second.file
            << " on line " << it->second.line << '\n';
        }
        it->second.is_freed = true;
        freed_temp_zone.emplace_back(it->second);
        if(freed_temp_zone.size()>=64) {
            freed_temp_zone.erase(freed_temp_zone.begin());
        }
        std::free(ptr);
        allocations.erase(it);
        tracking = false;
        return;
    }
}


#define new new(__FILE__, __LINE__)

#endif