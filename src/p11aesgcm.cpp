// -*- mode: c++; c-file-style:"stroustrup"; -*-
// p11aesgcm: AES Gallois Counter Mode

#include "p11aesgcm.hpp"
#include <iostream>
#include <random>
#include <algorithm>

P11AESGCMBenchmark::P11AESGCMBenchmark(const std::string &label, const Implementation::Vendor vendor) :
  P11Benchmark( "AES Authenticated Encryption (CKM_AES_GCM)", label, ObjectClass::SecretKey, vendor ) { }


P11AESGCMBenchmark::P11AESGCMBenchmark(const P11AESGCMBenchmark &other) :
    P11Benchmark(other) { }


inline P11AESGCMBenchmark *P11AESGCMBenchmark::clone() const {
    return new P11AESGCMBenchmark{*this};
}


void P11AESGCMBenchmark::prepare(Session &session, Object &obj, std::optional<size_t> threadindex)
{

    // prepare gcm_param
    switch(flavour()) {
    case Implementation::Vendor::generic:
    {
	// IV is 12 bytes wide
	// payload becomes [ PAYLOAD | AUTH (16 bytes) ]

	m_iv.resize(12);

	// fill m_iv with random
	std::random_device rd;
	std::mt19937 g(rd());
	std::shuffle(m_iv.begin(), m_iv.end(), g);

	// adjust GCM_PARAMS accordingly

	m_gcm_params.pIv = m_iv.data();
	m_gcm_params.ulIvLen = m_iv.size();
	m_gcm_params.ulIvBits = m_iv.size() << 3;

	m_encrypted.resize( m_payload.size() + 16 );

	break;
    }
    
    case Implementation::Vendor::luna:
	// payload [ PAYLOAD | AUTH (16 bytes) | IV (16 bytes) ]
	// IV is always 16 bytes
	// and is appended to the output

	m_iv.resize(16);

	m_encrypted.resize( m_payload.size() + 32 ); // should be 16, but on Safenet in FIPS mode, IV is also returned,
                                                 // which makes an additional 16 bytes
	
	break;

    case Implementation::Vendor::utimaco:
    case Implementation::Vendor::entrust:
    {
	// IV is 12 bytes wide
	// and MUST be filled with 0x00
	// payload becomes [ PAYLOAD | AUTH (16 bytes) ]

	m_iv.resize(12);
	std::fill(m_iv.begin(), m_iv.end(), 0);

	// adjust GCM_PARAMS accordingly

	m_gcm_params.pIv = m_iv.data();
	m_gcm_params.ulIvLen = m_iv.size();
	m_gcm_params.ulIvBits = m_iv.size() << 3;

	m_encrypted.resize( m_payload.size() + 16 ); // we request an authtag of 16 bytes

	break;
    }

    default:
	std::cerr << "Unsupported flavour for GCM\n";
	throw std::string("Unsupported architecture");
    }

    m_objhandle = obj.handle();
}

void P11AESGCMBenchmark::crashtestdummy(Session &session)
{
    Ulong returned_len=m_encrypted.size();
    session.module()->C_EncryptInit(session.handle(), &m_mech_aes_gcm, m_objhandle);
    session.module()->C_Encrypt( session.handle(), m_payload.data(), m_payload.size(), m_encrypted.data(), &returned_len);
}

