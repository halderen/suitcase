-- qschema
SELECT count(name) FROM sqlite_master WHERE type='table' AND name='properties';

-- schema
CREATE TABLE properties (
  propertyName VARCHAR(32),
  propertyValue VARCHAR(255)
);

CREATE UNIQUE INDEX propertyIndex ON properties ( propertyName );

CREATE TABLE policy (
  id INT NOT NULL,
  name TEXT NOT NULL
);

CREATE UNIQUE INDEX policyIndex ON policy ( id );

CREATE TABLE zone (
  id INT NOT NULL,
  name TEXT NOT NULL,
  revision INT NOT NULL default 1,
  policy INT NOT NULL,
  parent INT default NULL
);

CREATE UNIQUE INDEX zoneIndex on zone ( id );

INSERT INTO properties VALUES ( 'version', 4 );

INSERT INTO policy VALUES ( 0, "default" );

INSERT INTO policy VALUES ( 1, "experimental" );

INSERT INTO zone VALUES ( 0, "example.com", 1, 0, NULL );

INSERT INTO zone VALUES ( 1, "example.org", 1, 0, NULL );

INSERT INTO zone VALUES ( 2, "opendnssec.org", 1, 1, NULL );

INSERT INTO zone VALUES ( 3, "opendnssec.com", 1, 1, 2 );

UPDATE properties SET propertyValue = 3 WHERE propertyName = 'version';

-- schema1
CREATE TABLE policy (
  id INT NOT NULL,
  name TEXT NOT NULL
);

CREATE UNIQUE INDEX policyIndex ON policy ( id );

-- qschema1
SELECT count(propertyName) FROM properties WHERE propertyName='version' AND propertyValue >= 3;

-- schema2
CREATE TABLE zone (
  id INT NOT NULL,
  name TEXT NOT NULL,
  revision INT NOT NULL default 1,
  policy INT NOT NULL,
  parent INT default NULL
);

CREATE UNIQUE INDEX zoneIndex on policy ( id );

UPDATE properties SET propertyValue = 4 WHERE propertyName = 'version';

-- qschema2
SELECT count(propertyName) FROM properties WHERE propertyName='version' AND propertyValue >= 4;

-- fetchall
SELECT id, name FROM policy ORDER BY name;

SELECT id, name, policy, parent FROM zone ORDER BY name;

-- update-policy
UPDATE policy SET name = ? WHERE id = ?

-- update-zone
DELETE FROM zone WHERE id = ? AND revision = ?;

INSERT INTO zone ( id, revision, name, policy ) VALUES ( ?, ?, ?, ? );

-- default
SELECT id, name FROM policy ORDER BY name;

SELECT id, revision, name, policy, policy, parent, parent FROM zone ORDER BY name;
