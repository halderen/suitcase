const char* const schema = 
  "CREATE TABLE properties (\n"
  "  propertyName VARCHAR(32),\n"
  "  propertyValue VARCHAR(255)\n"
  ")";

const char* const uprade = 
  "CREATE INDEX propertyIndex ON propertyName";

const char* const setup = 
  "INSERT INTO properties VALUES ( 'version', 1 )";

const char* const qversion = 
  "SELECT propertyValue FROM properties WHERE propertyName = 'version'";

const char* sqlstmts_sqlite[][2] = {
  { "qversion", qversion },
  { "uprade", uprade },
  { "schema", schema },
  { "setup", setup },
  { NULL, NULL }
};
