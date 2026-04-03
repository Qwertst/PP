#include "worker.hpp"
#include "key.hpp"
#include <openssl/evp.h>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <pthread.h>
#include <vector>
#include <mutex>

namespace {

std::vector<uint8_t> evp_digest(const EVP_MD* md, const uint8_t* data, size_t len) {
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) throw std::runtime_error("EVP_MD_CTX_new failed");

    unsigned int out_len = 0;
    std::vector<uint8_t> out(EVP_MD_size(md));

    EVP_DigestInit_ex(ctx, md, nullptr);
    EVP_DigestUpdate(ctx, data, len);
    EVP_DigestFinal_ex(ctx, out.data(), &out_len);
    EVP_MD_CTX_free(ctx);

    out.resize(out_len);
    return out;
}

std::string compute_hash(const Key& key) {
    const auto& s = key.str();
    auto md5 = evp_digest(EVP_md5(),
        reinterpret_cast<const uint8_t*>(s.data()), s.size());
    auto sha1 = evp_digest(EVP_sha1(), md5.data(), md5.size());

    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (auto b : sha1) {
        oss << std::setw(2) << static_cast<unsigned>(b);
    }
    return oss.str();
}

struct ThreadArgs {
    Key start;
    Key end;
    std::string target_hash;
    std::atomic<bool>* stop_flag;
    std::mutex* result_mutex;
    SearchResult* result;
};

void* thread_func(void* arg) {
    auto* a = static_cast<ThreadArgs*>(arg);
    Key key = a->start;

    while (!a->stop_flag->load(std::memory_order_relaxed)) {
        if (compute_hash(key) == a->target_hash) {
            std::lock_guard<std::mutex> lock(*a->result_mutex);
            if (!a->result->found) {
                a->result->found = true;
                a->result->key = key.str();
            }
            a->stop_flag->store(true, std::memory_order_relaxed);
            break;
        }
        if (key == a->end) break;
        if (!key.increment()) break;
        if (!(key <= a->end)) break;
    }

    return nullptr;
}

} // namespace

Worker::Worker(int num_threads) : num_threads_(num_threads > 0 ? num_threads : 1) {}

int Worker::num_threads() const {
    return num_threads_;
}

SearchResult Worker::search(const SearchTask& task, std::atomic<bool>& stop_flag) {
    SearchResult result{false, ""};

    Key start(task.start);
    Key end(task.end);

    if (!(start <= end)) return result;

    std::mutex result_mutex;

    std::vector<Key> seg_starts;
    std::vector<Key> seg_ends;

    Key cur = start;
    for (int t = 0; t < num_threads_; ++t) {
        if (!(cur <= end)) break;
        seg_starts.push_back(cur);
        if (t == num_threads_ - 1) {
            seg_ends.push_back(end);
        } else {
            Key next_start = cur.advanced(1000000ULL);
            if (!(next_start <= end)) {
                seg_ends.push_back(end);
                cur = end.advanced(1);
            } else {
                Key seg_end = cur.advanced(999999ULL);
                if (!(seg_end <= end)) seg_end = end;
                seg_ends.push_back(seg_end);
                cur = next_start;
            }
        }
    }

    int actual = static_cast<int>(seg_starts.size());
    std::vector<ThreadArgs> args;
    args.reserve(actual);
    std::vector<pthread_t> threads(actual);

    for (int i = 0; i < actual; ++i) {
        args.push_back({seg_starts[i], seg_ends[i], task.target_hash, &stop_flag, &result_mutex, &result});
        pthread_create(&threads[i], nullptr, thread_func, &args[i]);
    }

    for (int i = 0; i < actual; ++i) {
        pthread_join(threads[i], nullptr);
    }

    return result;
}
