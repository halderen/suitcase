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

INSERT INTO policy VALUES ( 0, 'default' );

CREATE TABLE zone (
  id INT NOT NULL,
  name TEXT NOT NULL,
  revision INT NOT NULL default 1,
  policy INT NOT NULL,
  parent INT default NULL
);

CREATE UNIQUE INDEX zoneIndex on policy ( id );

INSERT INTO properties VALUES ( 'version', 4 );

-- schema1
CREATE TABLE policy (
  id INT NOT NULL,
  name TEXT NOT NULL
);

CREATE UNIQUE INDEX policyIndex ON policy ( id );

INSERT INTO policy VALUES ( 0, "default" );

UPDATE properties SET propertyValue = 3 WHERE propertyName = 'version';

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
DELETE FROM zone WHERE id = ? AND revision = ?

INSERT INTO zone VALUES ( ?, ?, ?, ?, ? );
