-- qschema
SELECT count(name) FROM sqlite_master WHERE type='table' AND name='properties';

-- schema
CREATE TABLE properties (
  propertyName VARCHAR(32),
  propertyValue VARCHAR(255),
  PRIMARY KEY ( propertyName )
);
INSERT INTO properties VALUES ( 'version', 4 );
CREATE TABLE policy (
  id INT NOT NULL,
  name TEXT NOT NULL,
  PRIMARY KEY ( id ),
  UNIQUE KEY ( name )
);
CREATE TABLE zone (
  id INT NOT NULL,
  name TEXT NOT NULL,
  revision INT NOT NULL default 1,
  policy INT NOT NULL,
  parent INT DEFAULT NULL,
  PRIMARY KEY ( id ),
  UNIQUE KEY ( name )
);
INSERT INTO policy VALUES ( 0, "default" );
INSERT INTO policy VALUES ( 1, "experimental" );
INSERT INTO zone VALUES ( 0, "example.com", 1, 0, NULL );
INSERT INTO zone VALUES ( 1, "example.org", 1, 0, NULL );
INSERT INTO zone VALUES ( 2, "opendnssec.org", 1, 1, NULL );
INSERT INTO zone VALUES ( 3, "opendnssec.com", 1, 1, 2 );

-- qschema1
SELECT count(propertyName) FROM properties WHERE propertyName='version' AND propertyValue >= 4;

-- schema1
UPDATE properties SET propertyValue = 4 WHERE propertyName = 'version';

-- default
SELECT id, name FROM policy ORDER BY name;

SELECT id, revision, name, policy, policy, parent, parent FROM zone ORDER BY name;

UPDATE policy SET name = ? WHERE id = ?;

DELETE FROM policy WHERE id = ?;

INSERT INTO zone ( id, revision, name, policy, parent ) VALUES ( ?, ?, ?, ?, ? );

DELETE FROM zone WHERE id = ? AND revision = ?;

-- probe
SELECT 1;
