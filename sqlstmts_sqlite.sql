-- schema
CREATE TABLE properties (
  propertyName VARCHAR(32),
  propertyValue VARCHAR(255)
);

-- uprade
CREATE INDEX propertyIndex ON propertyName;

-- setup
INSERT INTO properties VALUES ( 'version', 1 );

-- qversion
SELECT propertyValue FROM properties WHERE propertyName = 'version';
