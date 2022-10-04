// -*- mode: c++; c-file-style:"stroustrup"; -*-

#if !defined(P11BENCHMARK_H)
#define P11BENCHMARK_H

#include <forward_list>
#include <optional>
#include <utility>
#include <botan/auto_rng.h>
#include <botan/p11_types.h>
#include <botan/p11_object.h>
#include <botan/p11_rsa.h>
#include <botan/p11_ecdsa.h>
#include <botan/pubkey.h>
#include <boost/timer/timer.hpp>
#include "implementation.hpp"
#include "../config.h"


using namespace Botan::PKCS11;
using namespace boost::timer;
using benchmark_result_t = std::pair<std::vector<nanosecond_type>,int>;

class P11Benchmark
{
    std::string m_name;
    std::string m_label;
    ObjectClass m_objectclass;
    Implementation m_implementation;

protected:
    std::vector<uint8_t> m_payload;

    // prepare(): prepare calls to crashtestdummy() with object found
    virtual void prepare(Session &session, Object &obj, std::optional<size_t> threadindex)=0;

    // crashtestdummy(): here lies actual PKCS#11 calls to measure
    virtual void crashtestdummy(Session &session)=0;

    // cleanup(): perform cleanup after each call of crashtestdummy(), if needed
    virtual void cleanup(Session &session) { };

    // rename(): change the name of the class after creation
    inline void rename(std::string newname) { m_name = newname; };

    // build_threaded_label(): build label with thread index
    std::string build_threaded_label(std::optional<size_t> threadindex);

    // flavour(): returns which PKCS#11 flavour is selected
    inline Implementation::Vendor flavour() {return m_implementation.vendor(); };

public:
    P11Benchmark(const std::string &name,
		 const std::string &label,
		 ObjectClass objectclass,
		 const Implementation::Vendor vendor = Implementation::Vendor::generic
	);

    P11Benchmark(const P11Benchmark& other);
    P11Benchmark& operator=(const P11Benchmark& other);

    virtual ~P11Benchmark() { };

    // clone() is used by assignment operator to allow copy of the object
    virtual P11Benchmark *clone() const = 0;

    inline std::string name() const { return m_name; }
    inline std::string label() const { return m_label; }

    virtual std::string features() const;

    benchmark_result_t execute(Session* session, const std::vector<uint8_t> &payload, unsigned long iterations, std::optional<size_t> threadindex);

};


#endif // P11BENCHMARK_H
