#pragma once

#include <sqlite3.h>
#include <atomic>
#include <string>  //  std::string

#include "error_code.h"

namespace sqlite_orm {

    namespace internal {

        struct connection_holder {

            connection_holder(std::string filename_, std::string vfs_name_ = {}) :
                filename(std::move(filename_)), vfs_name(std::move(vfs_name_)) {}

            void retain() {
                if(1 == ++this->_retain_count) {

                    const char * vfs = vfs_name.empty() ? nullptr : vfs_name.c_str();

                    auto rc = sqlite3_open_v2(this->filename.c_str(),
                                              &this->db,
                                              SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
                                              vfs);

                    if(rc != SQLITE_OK) {
                        throw_translated_sqlite_error(db);
                    }
                }
            }

            void release() {
                if(0 == --this->_retain_count) {
                    auto rc = sqlite3_close_v2(this->db);
                    if(rc != SQLITE_OK) {
                        throw_translated_sqlite_error(db);
                    }
                }
            }

            sqlite3* get() const {
                return this->db;
            }

            int retain_count() const {
                return this->_retain_count;
            }

            const std::string filename;
            const std::string vfs_name;

          protected:
            sqlite3* db = nullptr;
            std::atomic_int _retain_count{};
        };

        struct connection_ref {
            connection_ref(connection_holder& holder) : holder(&holder) {
                this->holder->retain();
            }

            connection_ref(const connection_ref& other) : holder(other.holder) {
                this->holder->retain();
            }

            // rebind connection reference
            connection_ref& operator=(const connection_ref& other) {
                if(other.holder != this->holder) {
                    this->holder->release();
                    this->holder = other.holder;
                    this->holder->retain();
                }

                return *this;
            }

            ~connection_ref() {
                this->holder->release();
            }

            sqlite3* get() const {
                return this->holder->get();
            }

          private:
            connection_holder* holder = nullptr;
        };
    }
}
