PRAGMA journal_mode = WAL;
PRAGMA synchronous = OFF;

CREATE TABLE properties (
  propertyName VARCHAR(32),
  propertyValue VARCHAR(255)
);

INSERT INTO  properties VALUES ( 'version', 1 );
