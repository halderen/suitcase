-- qschema
SELECT count(name) FROM sqlite_master WHERE type='table' AND name='databaseVersion';

-- schema
CREATE TABLE databaseVersion (
  id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
  rev INTEGER NOT NULL DEFAULT 1,
  version UNSIGNED INTEGER NOT NULL
);

CREATE TABLE policy (
    id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
    rev INTEGER NOT NULL DEFAULT 1,
    name TEXT NOT NULL,
    description TEXT NOT NULL,
    signaturesResign UNSIGNED INT NOT NULL,
    signaturesRefresh UNSIGNED INT NOT NULL,
    signaturesJitter UNSIGNED INT NOT NULL,
    signaturesInceptionOffset UNSIGNED INT NOT NULL,
    signaturesValidityDefault UNSIGNED INT NOT NULL,
    signaturesValidityDenial UNSIGNED INT NOT NULL,
    signaturesValidityKeyset UNSIGNED INT,
    signaturesMaxZoneTtl UNSIGNED INT NOT NULL,
    denialType INT NOT NULL,
    denialOptout UNSIGNED INT NOT NULL,
    denialTtl UNSIGNED INT NOT NULL,
    denialResalt UNSIGNED INT NOT NULL,
    denialAlgorithm UNSIGNED INT NOT NULL,
    denialIterations UNSIGNED INT NOT NULL,
    denialSaltLength UNSIGNED INT NOT NULL,
    denialSalt TEXT NOT NULL,
    denialSaltLastChange UNSIGNED INT NOT NULL,
    keysTtl UNSIGNED INT NOT NULL,
    keysRetireSafety UNSIGNED INT NOT NULL,
    keysPublishSafety UNSIGNED INT NOT NULL,
    keysShared UNSIGNED INT NOT NULL,
    keysPurgeAfter UNSIGNED INT NOT NULL,
    zonePropagationDelay UNSIGNED INT NOT NULL,
    zoneSoaTtl UNSIGNED INT NOT NULL,
    zoneSoaMinimum UNSIGNED INT NOT NULL,
    zoneSoaSerial INT NOT NULL,
    parentRegistrationDelay UNSIGNED INT NOT NULL,
    parentPropagationDelay UNSIGNED INT NOT NULL,
    parentDsTtl UNSIGNED INT NOT NULL,
    parentSoaTtl UNSIGNED INT NOT NULL,
    parentSoaMinimum UNSIGNED INT NOT NULL,
    passthrough UNSIGNED INT NOT NULL
);

CREATE UNIQUE INDEX policyName ON policy ( name );

CREATE TABLE policyKey (
    id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
    rev INTEGER NOT NULL DEFAULT 1,
    policyId INTEGER NOT NULL,
    role INT NOT NULL,
    algorithm UNSIGNED INT NOT NULL,
    bits UNSIGNED INT NOT NULL,
    lifetime UNSIGNED INT NOT NULL,
    repository TEXT NOT NULL,
    standby UNSIGNED INT NOT NULL,
    manualRollover UNSIGNED INT NOT NULL,
    rfc5011 UNSIGNED INT NOT NULL,
    minimize UNSIGNED INT NOT NULL
);

CREATE INDEX policyKeyPolicyId ON policyKey ( policyId );

CREATE TABLE hsmKey (
    id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
    rev INTEGER NOT NULL DEFAULT 1,
    policyId INTEGER NOT NULL,
    locator TEXT NOT NULL,
    state INT NOT NULL,
    bits UNSIGNED INT NOT NULL,
    algorithm UNSIGNED INT NOT NULL,
    role INT NOT NULL,
    inception UNSIGNED INT NOT NULL,
    isRevoked UNSIGNED INT NOT NULL,
    keyType INT NOT NULL,
    repository TEXT NOT NULL,
    backup INT NOT NULL
);

CREATE INDEX hsmKeyPolicyId ON hsmKey ( policyId );

CREATE UNIQUE INDEX hsmKeyLocator ON hsmKey ( locator );

CREATE TABLE zone (
    id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
    rev INTEGER NOT NULL DEFAULT 1,
    policyId INTEGER NOT NULL,
    name TEXT NOT NULL,
    signconfNeedsWriting UNSIGNED INT NOT NULL,
    signconfPath TEXT NOT NULL,
    nextChange INT NOT NULL,
    ttlEndDs UNSIGNED INT NOT NULL,
    ttlEndDk UNSIGNED INT NOT NULL,
    ttlEndRs UNSIGNED INT NOT NULL,
    rollKskNow UNSIGNED INT NOT NULL,
    rollZskNow UNSIGNED INT NOT NULL,
    rollCskNow UNSIGNED INT NOT NULL,
    inputAdapterType TEXT NOT NULL,
    inputAdapterUri TEXT NOT NULL,
    outputAdapterType TEXT NOT NULL,
    outputAdapterUri TEXT NOT NULL,
    nextKskRoll UNSIGNED INT NOT NULL,
    nextZskRoll UNSIGNED INT NOT NULL,
    nextCskRoll UNSIGNED INT NOT NULL
);

CREATE INDEX zonePolicyId ON zone ( policyId );

CREATE UNIQUE INDEX zoneName ON zone ( name );

CREATE TABLE keyData (
    id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
    rev INTEGER NOT NULL DEFAULT 1,
    zoneId INTEGER NOT NULL,
    hsmKeyId INTEGER NOT NULL,
    algorithm UNSIGNED INT NOT NULL,
    inception UNSIGNED INT NOT NULL,
    role INT NOT NULL,
    introducing UNSIGNED INT NOT NULL,
    shouldRevoke UNSIGNED INT NOT NULL,
    standby UNSIGNED INT NOT NULL,
    activeZsk UNSIGNED INT NOT NULL,
    publish UNSIGNED INT NOT NULL,
    activeKsk UNSIGNED INT NOT NULL,
    dsAtParent INT NOT NULL,
    keytag UNSIGNED INT NOT NULL,
    minimize UNSIGNED INT NOT NULL
);

CREATE INDEX keyDataZoneId ON keyData ( zoneId );

CREATE INDEX keyDataHsmKeyId ON keyData ( hsmKeyId );

CREATE TABLE keyState (
    id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
    rev INTEGER NOT NULL DEFAULT 1,
    keyDataId INTEGER NOT NULL,
    type INT NOT NULL,
    state INT NOT NULL,
    lastChange UNSIGNED INT NOT NULL,
    minimize UNSIGNED INT NOT NULL,
    ttl UNSIGNED INT NOT NULL
);

CREATE INDEX keyStateKeyDataId ON keyState ( keyDataId );

CREATE TABLE keyDependency (
    id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
    rev INTEGER NOT NULL DEFAULT 1,
    zoneId INTEGER NOT NULL,
    fromKeyDataId INTEGER NOT NULL,
    toKeyDataId INTEGER NOT NULL,
    type INT NOT NULL
);

CREATE INDEX keyDependencyZoneId ON keyDependency ( zoneId );

CREATE INDEX keyDependencyFromKeyDataId ON keyDependency ( fromKeyDataId );

CREATE INDEX keyDependencyToKeyDataId ON keyDependency ( toKeyDataId );

-- schema1
DROP INDEX keyDependencyZoneId;

ALTER TABLE keyDependency DROP COLUMN zoneId;

UPDATE databaseVersion set version = 2;

-- qschema1
SELECT count(version) FROM databaseVersion WHERE version >= 2;

-- default
SELECT id, rev, locator, repository, state, bits, algorithm, role, inception, isRevoked, keyType, backup, policyId FROM hsmKey;

INSERT OR REPLACE INTO hsmKey ( id, rev, policyId, locator, state, bits, algorithm, role, inception, isRevoked, keyType, repository, backup )
VALUES ( ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);

DELETE FROM hsmKey WHERE id = ? AND rev = ?;

SELECT id, rev, name, description, passthrough,
       signaturesResign, signaturesRefresh, signaturesJitter, signaturesInceptionOffset,
       signaturesValidityDefault, signaturesValidityDenial, signaturesValidityKeyset, signaturesMaxZoneTtl,
       denialType, denialOptout, denialTtl, denialResalt, denialAlgorithm, denialIterations,
       denialSaltLength, denialSalt, denialSaltLastChange,
       keysTtl, keysRetireSafety, keysPublishSafety, keysShared, keysPurgeAfter,
       zonePropagationDelay, zoneSoaTtl, zoneSoaMinimum, zoneSoaSerial,
       parentRegistrationDelay, parentPropagationDelay, parentDsTtl, parentSoaTtl, parentSoaMinimum
FROM policy;

INSERT OR REPLACE INTO policy ( id, rev, name, description, passthrough,
       signaturesResign, signaturesRefresh, signaturesJitter, signaturesInceptionOffset,
       signaturesValidityDefault, signaturesValidityDenial, signaturesValidityKeyset, signaturesMaxZoneTtl,
       denialType, denialOptout, denialTtl, denialResalt, denialAlgorithm, denialIterations,
       denialSaltLength, denialSalt, denialSaltLastChange,
       keysTtl, keysRetireSafety, keysPublishSafety, keysShared, keysPurgeAfter,
       zonePropagationDelay, zoneSoaTtl, zoneSoaMinimum, zoneSoaSerial,
       parentRegistrationDelay, parentPropagationDelay, parentDsTtl, parentSoaTtl, parentSoaMinimum )
VALUES ( ?, ?, ?, ?, ?,
         ?, ?, ?, ?, ?, ?, ?, ?,
         ?, ?, ?, ?, ?, ?, ?, ?, ?,
         ?, ?, ?, ?, ?, ?, ?, ?, ?,
         ?, ?, ?, ?, ? );

DELETE FROM policy WHERE id = ? AND rev = ?;

SELECT id, rev, policyId, policyId, repository, role, algorithm, bits, lifetime, standby, manualRollover, rfc5011, minimize
FROM policyKey;

INSERT OR REPLACE INTO policyKey ( id, rev, policyId, role, algorithm, bits, lifetime, repository, standby, manualRollover, rfc5011, minimize )
VALUES ( ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ? );

DELETE FROM policykey WHERE id = ? AND rev = ?;

SELECT id, rev, policyId, policyId, name, nextChange, signconfPath,
       inputAdapterUri, inputAdapterType, outputAdapterUri, outputAdapterType,
       nextKskRoll, nextZskRoll, nextCskRoll, signconfNeedsWriting,
       ttlEndDs, ttlEndDk, ttlEndRs, rollKskNow, rollZskNow, rollCskNow
FROM zone;

INSERT OR REPLACE INTO zone ( id, rev, policyId, name, nextChange, signconfPath,
       inputAdapterUri, inputAdapterType, outputAdapterUri, outputAdapterType,
       nextKskRoll, nextZskRoll, nextCskRoll, signconfNeedsWriting,
       ttlEndDs, ttlEndDk, ttlEndRs, rollKskNow, rollZskNow, rollCskNow )
VALUES ( ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)

DELETE FROM zone WHERE id = ? AND rev = ?;

SELECT id, rev, zoneId, zoneId, hsmKeyId,
       algorithm, inception, role, introducing, shouldRevoke, standby,
       activeZsk, publish, activeKsk, dsAtParent, keytag, minimize
FROM keyData;

INSERT OR REPLACE INTO keyData ( id, rev, zoneId, hsmKeyId,
       algorithm, inception, role, introducing, shouldRevoke, standby,
       activeZsk, publish, activeKsk, dsAtParent, keytag, minimize )
VALUES ( ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ? );

DELETE FROM keyData WHERE id = ? AND rev = ?;

SELECT id, rev, keyDataId, type, state, lastChange, minimize, ttl
FROM keyState;

INSERT OR REPLACE INTO keyState ( id, rev, keyDataId, type, state, lastChange, minimize, ttl )
VALUES ( ?, ?, ?, ?, ?, ?, ?, ? );

DELETE FROM keyState WHERE id = ? AND rev = ?;

SELECT id, rev, fromKeyDataId, toKeyDataId, type
FROM keyDependency;

INSERT OR REPLACE INTO keyDependency ( id, rev, fromKeyDataId, toKeyDataId, type )
VALUES ( ?, ?, ?, ?, ? );

DELETE FROM keyDependency WHERE id = ? AND rev = ?;

-- probe
SELECT 1;
