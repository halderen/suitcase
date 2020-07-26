const char* const sqlstmts_sqlite_qschema_[] = {
  "SELECT count(name) FROM sqlite_master WHERE type='table' AND name='properties'",

  NULL };

const char* const sqlstmts_sqlite_schema_[] = {
  "CREATE TABLE properties (\n"
  "  propertyName VARCHAR(32),\n"
  "  propertyValue VARCHAR(255)\n"
  ")",

  "CREATE UNIQUE INDEX propertyIndex ON properties ( propertyName )",

  "CREATE TABLE policy (\n"
  "  id INT NOT NULL,\n"
  "  name TEXT NOT NULL\n"
  ")",

  "CREATE UNIQUE INDEX policyIndex ON policy ( id )",

  "CREATE TABLE zone (\n"
  "  id INT NOT NULL,\n"
  "  name TEXT NOT NULL,\n"
  "  revision INT NOT NULL default 1,\n"
  "  policy INT NOT NULL,\n"
  "  parent INT default NULL\n"
  ")",

  "CREATE UNIQUE INDEX zoneIndex on zone ( id )",

  "INSERT INTO properties VALUES ( 'version', 4 )",

  "INSERT INTO policy VALUES ( 0, \"default\" )",

  "INSERT INTO policy VALUES ( 1, \"experimental\" )",

  "INSERT INTO zone VALUES ( 0, \"example.com\", 1, 0, NULL )",

  "INSERT INTO zone VALUES ( 1, \"example.org\", 1, 0, NULL )",

  "INSERT INTO zone VALUES ( 2, \"opendnssec.org\", 1, 1, NULL )",

  "INSERT INTO zone VALUES ( 3, \"opendnssec.com\", 1, 1, 2 )",

  "UPDATE properties SET propertyValue = 3 WHERE propertyName = 'version'",

  NULL };

const char* const sqlstmts_sqlite_schema1_[] = {
  "CREATE TABLE policy (\n"
  "  id INT NOT NULL,\n"
  "  name TEXT NOT NULL\n"
  ")",

  "CREATE UNIQUE INDEX policyIndex ON policy ( id )",

  NULL };

const char* const sqlstmts_sqlite_qschema1_[] = {
  "SELECT count(propertyName) FROM properties WHERE propertyName='version' AND propertyValue >= 3",

  NULL };

const char* const sqlstmts_sqlite_schema2_[] = {
  "CREATE TABLE zone (\n"
  "  id INT NOT NULL,\n"
  "  name TEXT NOT NULL,\n"
  "  revision INT NOT NULL default 1,\n"
  "  policy INT NOT NULL,\n"
  "  parent INT default NULL\n"
  ")",

  "CREATE UNIQUE INDEX zoneIndex on policy ( id )",

  "UPDATE properties SET propertyValue = 4 WHERE propertyName = 'version'",

  NULL };

const char* const sqlstmts_sqlite_qschema2_[] = {
  "SELECT count(propertyName) FROM properties WHERE propertyName='version' AND propertyValue >= 4",

  NULL };

const char* const sqlstmts_sqlite_default_[] = {
  "SELECT id, name FROM policy ORDER BY name",

  "UPDATE policy SET name = ? WHERE id = ?",

  "DELETE FROM policy WHERE id = ?",

  "SELECT id, revision, name, policy, policy, parent, parent FROM zone ORDER BY name",

  "INSERT INTO zone ( id, revision, name, policy ) VALUES ( ?, ?, ?, ? )",

  "DELETE FROM zone WHERE id = ? AND revision = ?",

  NULL };

const char* const* sqlstmts_sqlite_array[] = {
  sqlstmts_sqlite_schema2_,
  sqlstmts_sqlite_qschema_,
  sqlstmts_sqlite_qschema1_,
  sqlstmts_sqlite_qschema2_,
  sqlstmts_sqlite_default_,
  sqlstmts_sqlite_schema_,
  sqlstmts_sqlite_schema1_,
  NULL 
};

const char* const** sqlstmts_sqlite_schema2 = &sqlstmts_sqlite_array[0];
const char* const** sqlstmts_sqlite_qschema = &sqlstmts_sqlite_array[1];
const char* const** sqlstmts_sqlite_qschema1 = &sqlstmts_sqlite_array[2];
const char* const** sqlstmts_sqlite_qschema2 = &sqlstmts_sqlite_array[3];
const char* const** sqlstmts_sqlite_default = &sqlstmts_sqlite_array[4];
const char* const** sqlstmts_sqlite_schema = &sqlstmts_sqlite_array[5];
const char* const** sqlstmts_sqlite_schema1 = &sqlstmts_sqlite_array[6];
