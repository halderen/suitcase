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

-- default
SELECT id, name FROM policy ORDER BY name;

UPDATE policy SET name = ? WHERE id = ?;

DELETE FROM policy WHERE id = ?;

SELECT id, revision, name, policy, policy, parent, parent FROM zone ORDER BY name;

INSERT INTO zone ( id, revision, name, policy ) VALUES ( ?, ?, ?, ? );

DELETE FROM zone WHERE id = ? AND revision = ?;
