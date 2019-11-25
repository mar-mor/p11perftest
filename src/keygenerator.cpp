// keygenerator.cpp: a class to organize generation of keys on several threads
// -*- mode: c++; c-file-style:"stroustrup"; -*-

#include <iostream>
#include <future>
#include <array>
#include <botan/p11_rsa.h>
#include "keygenerator.hpp"

using namespace Botan::PKCS11;


bool KeyGenerator::generate_rsa_keypair(std::string alias, unsigned int bits, Session *session)
{
    bool rv;
    try {
	Botan::PKCS11::RSA_PrivateKeyGenerationProperties priv_generate_props;
	priv_generate_props.set_token( false );
	priv_generate_props.set_private( true );
	priv_generate_props.set_sign( true );
	priv_generate_props.set_label( alias );

	Botan::PKCS11::RSA_PublicKeyGenerationProperties pub_generate_props( bits );
	pub_generate_props.set_pub_exponent(65537);
	pub_generate_props.set_label( alias );
	pub_generate_props.set_token( false );
	pub_generate_props.set_verify( true );
	pub_generate_props.set_private( false );

	Botan::PKCS11::PKCS11_RSA_KeyPair rsa_keypair = Botan::PKCS11::generate_rsa_keypair( *session, pub_generate_props, priv_generate_props );

	rv = true;
    } catch (Botan::Exception &bexc) {
	std::cerr << "ERROR:: caught an exception:" << bexc.what() << std::endl;
	// we print the exception, and move on
	rv = false;
    }
    return rv;
}

bool KeyGenerator::generate_des_key(std::string alias, unsigned int bits, Session *session)
{
    bool rv;
    Byte btrue = CK_TRUE;
    Byte bfalse = CK_FALSE;
    Ulong len = bits >> 3;
    ObjectHandle handle;
    Mechanism mechanism { CKM_DES_KEY_GEN, nullptr, 0 };

    switch(bits) {
	case 64:
	    mechanism.mechanism = CKM_DES_KEY_GEN;
	    break;

	case 128:
	    mechanism.mechanism = CKM_DES2_KEY_GEN;
	    break;

	case 192:
	    mechanism.mechanism = CKM_DES3_KEY_GEN;
	    break;

    default:
	std::cerr << "Invalid key length for DES:" << bits << std::endl;
	return false;
    }

    std::array<Attribute,5> keytemplate {
	{
	    { static_cast<CK_ATTRIBUTE_TYPE>(AttributeType::Label), const_cast< char* >(alias.c_str()), alias.size() },
	    { static_cast<CK_ATTRIBUTE_TYPE>(AttributeType::Token), &bfalse, sizeof(Byte) },
	    { static_cast<CK_ATTRIBUTE_TYPE>(AttributeType::Private), &btrue, sizeof(Byte) },
	    { static_cast<CK_ATTRIBUTE_TYPE>(AttributeType::Encrypt), &btrue, sizeof(Byte) },
	    { static_cast<CK_ATTRIBUTE_TYPE>(AttributeType::Decrypt), &btrue, sizeof(Byte) }
	}
    };

    try {
	session->module()->C_GenerateKey(session->handle(), &mechanism, keytemplate.data(), keytemplate.size(), &handle );
	rv = true;

    } catch (Botan::Exception &bexc) {
	std::cerr << "ERROR:: caught an exception:" << bexc.what() << std::endl;
	// we print the exception, and move on
	rv = false;
    }
    return rv;
}


bool KeyGenerator::generate_aes_key(std::string alias, unsigned int bits, Session *session)
{
    bool rv;
    Byte btrue = CK_TRUE;
    Byte bfalse = CK_FALSE;
    Ulong len = bits >> 3;
    ObjectHandle handle;
    Mechanism mech_aes_key_gen { CKM_AES_KEY_GEN, nullptr, 0 };

    if(bits != 256 && bits !=128 && bits != 192) {
	std::cerr << "Invalid key length for AES:" << bits << std::endl;
	return false;
    }

    std::array<Attribute,6> keytemplate {
	{
	    { static_cast<CK_ATTRIBUTE_TYPE>(AttributeType::Label), const_cast< char* >(alias.c_str()), alias.size() },
	    { static_cast<CK_ATTRIBUTE_TYPE>(AttributeType::Token), &bfalse, sizeof(Byte) },
	    { static_cast<CK_ATTRIBUTE_TYPE>(AttributeType::Private), &btrue, sizeof(Byte) },
	    { static_cast<CK_ATTRIBUTE_TYPE>(AttributeType::Encrypt), &btrue, sizeof(Byte) },
	    { static_cast<CK_ATTRIBUTE_TYPE>(AttributeType::Decrypt), &btrue, sizeof(Byte) },
	    { static_cast<CK_ATTRIBUTE_TYPE>(AttributeType::ValueLen), &len, sizeof(Ulong) }
	}
    };

    try {
	session->module()->C_GenerateKey(session->handle(), &mech_aes_key_gen, keytemplate.data(), keytemplate.size(), &handle );
	rv = true;

    } catch (Botan::Exception &bexc) {
	std::cerr << "ERROR:: caught an exception:" << bexc.what() << std::endl;
	// we print the exception, and move on
	rv = false;
    }

    return rv;
}



void KeyGenerator::generate_key(KeyGenerator::KeyType keytype, std::string alias, unsigned int bits)
{
    int th;
    bool rv = true;
    std::vector<std::future<bool> > future_array(m_numthreads);

    // because I'm lazy, let's use decltype() to define the function pointer...
    using fnptr = decltype( &KeyGenerator::generate_rsa_keypair );

    std::map< const KeyGenerator::KeyType, fnptr> fnmap {
	{ KeyType::RSA, &KeyGenerator::generate_rsa_keypair },
	{ KeyType::AES, &KeyGenerator::generate_aes_key } ,
	{ KeyType::DES, &KeyGenerator::generate_des_key }
    };

    auto chooser = [&fnmap](KeyGenerator::KeyType kt) { return fnmap.at(kt);  };


    for(th=0; th<m_numthreads;th++) {

	future_array[th] = std::async( std::launch::async,
	 			       chooser(keytype),
				       this,
				       alias,
				       bits,
	 			       m_sessions[th].get());
    }

    // recover futures. If one is false, return false
    // TODO: replace with exception

    for(th=0;th<m_numthreads;th++) {
	if(future_array[th].get() == false) {
	    throw KeyGenerationException();
	}
    }
}

