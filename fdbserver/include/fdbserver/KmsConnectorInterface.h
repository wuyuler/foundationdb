/*
 * KmsConnectorInterface.h
 *
 * This source file is part of the FoundationDB open source project
 *
 * Copyright 2013-2022 Apple Inc. and the FoundationDB project authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef FDBSERVER_KMSCONNECTORINTERFACE_H
#define FDBSERVER_KMSCONNECTORINTERFACE_H
#pragma once

#include "fdbrpc/fdbrpc.h"
#include "flow/Arena.h"
#include "flow/EncryptUtils.h"
#include "flow/FileIdentifier.h"
#include "flow/Trace.h"
#include "flow/flow.h"
#include "flow/network.h"
#include "fdbclient/BlobMetadataUtils.h"

struct KmsConnectorInterface {
	constexpr static FileIdentifier file_identifier = 2416711;
	RequestStream<ReplyPromise<Void>> waitFailure;
	RequestStream<struct KmsConnLookupEKsByKeyIdsReq> ekLookupByIds;
	RequestStream<struct KmsConnLookupEKsByDomainIdsReq> ekLookupByDomainIds;
	RequestStream<struct KmsConnBlobMetadataReq> blobMetadataReq;

	KmsConnectorInterface() {}

	UID id() const { return ekLookupByIds.getEndpoint().token; }
	template <class Archive>
	void serialize(Archive& ar) {
		if constexpr (!is_fb_function<Archive>) {
			ASSERT(ar.protocolVersion().isValid());
		}
		serializer(ar, waitFailure);
		if (Archive::isDeserializing) {
			ekLookupByIds =
			    RequestStream<struct KmsConnLookupEKsByKeyIdsReq>(waitFailure.getEndpoint().getAdjustedEndpoint(1));
			ekLookupByDomainIds =
			    RequestStream<struct KmsConnLookupEKsByDomainIdsReq>(waitFailure.getEndpoint().getAdjustedEndpoint(2));
			blobMetadataReq =
			    RequestStream<struct KmsConnBlobMetadataReq>(waitFailure.getEndpoint().getAdjustedEndpoint(3));
		}
	}

	void initEndpoints() {
		std::vector<std::pair<FlowReceiver*, TaskPriority>> streams;
		streams.push_back(waitFailure.getReceiver());
		streams.push_back(ekLookupByIds.getReceiver(TaskPriority::Worker));
		streams.push_back(ekLookupByDomainIds.getReceiver(TaskPriority::Worker));
		streams.push_back(blobMetadataReq.getReceiver(TaskPriority::Worker));
		FlowTransport::transport().addEndpoints(streams);
	}
};

struct EncryptCipherKeyDetailsRef {
	constexpr static FileIdentifier file_identifier = 1227025;
	EncryptCipherDomainId encryptDomainId;
	EncryptCipherBaseKeyId encryptKeyId;
	StringRef encryptKey;
	Optional<int64_t> refreshAfterSec;
	Optional<int64_t> expireAfterSec;

	EncryptCipherKeyDetailsRef() {}
	explicit EncryptCipherKeyDetailsRef(Arena& arena,
	                                    EncryptCipherDomainId dId,
	                                    EncryptCipherBaseKeyId keyId,
	                                    StringRef key)
	  : encryptDomainId(dId), encryptKeyId(keyId), encryptKey(StringRef(arena, key)),
	    refreshAfterSec(Optional<int64_t>()), expireAfterSec(Optional<int64_t>()) {}
	explicit EncryptCipherKeyDetailsRef(EncryptCipherDomainId dId, EncryptCipherBaseKeyId keyId, StringRef key)
	  : encryptDomainId(dId), encryptKeyId(keyId), encryptKey(key), refreshAfterSec(Optional<int64_t>()),
	    expireAfterSec(Optional<int64_t>()) {}
	explicit EncryptCipherKeyDetailsRef(Arena& arena,
	                                    EncryptCipherDomainId dId,
	                                    EncryptCipherBaseKeyId keyId,
	                                    StringRef key,
	                                    Optional<int64_t> refAfterSec,
	                                    Optional<int64_t> expAfterSec)
	  : encryptDomainId(dId), encryptKeyId(keyId), encryptKey(StringRef(arena, key)), refreshAfterSec(refAfterSec),
	    expireAfterSec(expAfterSec) {}
	explicit EncryptCipherKeyDetailsRef(EncryptCipherDomainId dId,
	                                    EncryptCipherBaseKeyId keyId,
	                                    StringRef key,
	                                    Optional<int64_t> refAfterSec,
	                                    Optional<int64_t> expAfterSec)
	  : encryptDomainId(dId), encryptKeyId(keyId), encryptKey(key), refreshAfterSec(refAfterSec),
	    expireAfterSec(expAfterSec) {}

	bool operator==(const EncryptCipherKeyDetailsRef& toCompare) {
		return encryptDomainId == toCompare.encryptDomainId && encryptKeyId == toCompare.encryptKeyId &&
		       encryptKey.compare(toCompare.encryptKey) == 0;
	}

	template <class Ar>
	void serialize(Ar& ar) {
		serializer(ar, encryptDomainId, encryptKeyId, encryptKey, refreshAfterSec, expireAfterSec);
	}
};

struct KmsConnLookupEKsByKeyIdsRep {
	constexpr static FileIdentifier file_identifier = 2313778;
	Arena arena;
	VectorRef<EncryptCipherKeyDetailsRef> cipherKeyDetails;

	KmsConnLookupEKsByKeyIdsRep() {}

	template <class Ar>
	void serialize(Ar& ar) {
		serializer(ar, cipherKeyDetails, arena);
	}
};

struct KmsConnLookupKeyIdsReqInfoRef {
	constexpr static FileIdentifier file_identifier = 3092256;
	EncryptCipherDomainId domainId;
	EncryptCipherBaseKeyId baseCipherId;
	EncryptCipherDomainNameRef domainName;

	KmsConnLookupKeyIdsReqInfoRef()
	  : domainId(INVALID_ENCRYPT_DOMAIN_ID), baseCipherId(INVALID_ENCRYPT_CIPHER_KEY_ID) {}
	explicit KmsConnLookupKeyIdsReqInfoRef(Arena& arena,
	                                       const EncryptCipherDomainId dId,
	                                       const EncryptCipherBaseKeyId bCId,
	                                       StringRef name)
	  : domainId(dId), baseCipherId(bCId), domainName(StringRef(arena, name)) {}

	bool operator==(const KmsConnLookupKeyIdsReqInfoRef& info) const {
		return domainId == info.domainId && baseCipherId == info.baseCipherId &&
		       (domainName.compare(info.domainName) == 0);
	}

	template <class Ar>
	void serialize(Ar& ar) {
		serializer(ar, domainId, baseCipherId, domainName);
	}
};

struct KmsConnLookupEKsByKeyIdsReq {
	constexpr static FileIdentifier file_identifier = 6913396;
	Arena arena;
	VectorRef<KmsConnLookupKeyIdsReqInfoRef> encryptKeyInfos;
	Optional<UID> debugId;
	ReplyPromise<KmsConnLookupEKsByKeyIdsRep> reply;

	KmsConnLookupEKsByKeyIdsReq() {}
	explicit KmsConnLookupEKsByKeyIdsReq(VectorRef<KmsConnLookupKeyIdsReqInfoRef> keyInfos, Optional<UID> dbgId)
	  : encryptKeyInfos(keyInfos), debugId(dbgId) {}

	template <class Ar>
	void serialize(Ar& ar) {
		serializer(ar, encryptKeyInfos, debugId, reply, arena);
	}
};

struct KmsConnLookupEKsByDomainIdsRep {
	constexpr static FileIdentifier file_identifier = 3009025;
	Arena arena;
	VectorRef<EncryptCipherKeyDetailsRef> cipherKeyDetails;

	KmsConnLookupEKsByDomainIdsRep() {}

	template <class Ar>
	void serialize(Ar& ar) {
		serializer(ar, cipherKeyDetails, arena);
	}
};

struct KmsConnLookupDomainIdsReqInfoRef {
	constexpr static FileIdentifier file_identifier = 8980149;
	EncryptCipherDomainId domainId;
	EncryptCipherDomainNameRef domainName;

	KmsConnLookupDomainIdsReqInfoRef() : domainId(INVALID_ENCRYPT_DOMAIN_ID) {}
	explicit KmsConnLookupDomainIdsReqInfoRef(Arena& arena, const KmsConnLookupDomainIdsReqInfoRef& from)
	  : domainId(from.domainId), domainName(StringRef(arena, from.domainName)) {}
	explicit KmsConnLookupDomainIdsReqInfoRef(Arena& arena, const EncryptCipherDomainId dId, StringRef name)
	  : domainId(dId), domainName(StringRef(arena, name)) {}
	explicit KmsConnLookupDomainIdsReqInfoRef(const EncryptCipherDomainId dId, StringRef name)
	  : domainId(dId), domainName(name) {}

	bool operator==(const KmsConnLookupDomainIdsReqInfoRef& info) const {
		return domainId == info.domainId && (domainName.compare(info.domainName) == 0);
	}

	template <class Ar>
	void serialize(Ar& ar) {
		serializer(ar, domainId, domainName);
	}
};

struct KmsConnLookupEKsByDomainIdsReq {
	constexpr static FileIdentifier file_identifier = 9918682;
	Arena arena;
	VectorRef<KmsConnLookupDomainIdsReqInfoRef> encryptDomainInfos;
	Optional<UID> debugId;
	ReplyPromise<KmsConnLookupEKsByDomainIdsRep> reply;

	KmsConnLookupEKsByDomainIdsReq() {}
	explicit KmsConnLookupEKsByDomainIdsReq(VectorRef<KmsConnLookupDomainIdsReqInfoRef>& infos, Optional<UID> dbgId)
	  : encryptDomainInfos(infos), debugId(dbgId) {}

	template <class Ar>
	void serialize(Ar& ar) {
		serializer(ar, encryptDomainInfos, debugId, reply, arena);
	}
};

struct KmsConnBlobMetadataRep {
	constexpr static FileIdentifier file_identifier = 2919714;
	Standalone<VectorRef<BlobMetadataDetailsRef>> metadataDetails;

	KmsConnBlobMetadataRep() {}

	template <class Ar>
	void serialize(Ar& ar) {
		serializer(ar, metadataDetails);
	}
};

struct KmsConnBlobMetadataReq {
	constexpr static FileIdentifier file_identifier = 3913147;
	Standalone<VectorRef<KmsConnLookupDomainIdsReqInfoRef>> domainInfos;
	Optional<UID> debugId;
	ReplyPromise<KmsConnBlobMetadataRep> reply;

	KmsConnBlobMetadataReq() {}

	template <class Ar>
	void serialize(Ar& ar) {
		serializer(ar, domainInfos, debugId, reply);
	}
};

#endif
