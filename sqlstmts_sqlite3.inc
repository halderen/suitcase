const char* const sqlstmts_sqlite3_qschema_[] = {
  "SELECT count(name) FROM sqlite_master WHERE type='table' AND name='properties'",

  NULL };

const char* const sqlstmts_sqlite3_schema_[] = {
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

  "UPDATE properties SET propertyValue = 3 WHERE propertyName = 'version'",

  NULL };

const char* const sqlstmts_sqlite3_schema1_[] = {
  "CREATE TABLE policy (\n"
  "  id INT NOT NULL,\n"
  "  name TEXT NOT NULL\n"
  ")",

  "CREATE UNIQUE INDEX policyIndex ON policy ( id )",

  NULL };

const char* const sqlstmts_sqlite3_qschema1_[] = {
  "SELECT count(propertyName) FROM properties WHERE propertyName='version' AND propertyValue >= 3",

  NULL };

const char* const sqlstmts_sqlite3_schema2_[] = {
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

const char* const sqlstmts_sqlite3_qschema2_[] = {
  "SELECT count(propertyName) FROM properties WHERE propertyName='version' AND propertyValue >= 4",

  NULL };

const char* const sqlstmts_sqlite3_default_[] = {
  "SELECT id, name FROM policy ORDER BY name",

  "UPDATE policy SET name = ? WHERE id = ?",

  "DELETE FROM policy WHERE id = ?",

  "SELECT id, revision, name, policy, policy, parent, parent FROM zone ORDER BY name",

  "INSERT INTO zone ( id, revision, name, policy ) VALUES ( ?, ?, ?, ? )",

  "DELETE FROM zone WHERE id = ? AND revision = ?",

  NULL };

const char* const* sqlstmts_sqlite3_array[] = {
  sqlstmts_sqlite3_schema2_,
  sqlstmts_sqlite3_qschema_,
  sqlstmts_sqlite3_qschema1_,
  sqlstmts_sqlite3_qschema2_,
  sqlstmts_sqlite3_default_,
  sqlstmts_sqlite3_schema_,
  sqlstmts_sqlite3_schema1_,
  NULL 
};

const char* const** sqlstmts_sqlite3_schema2 = &sqlstmts_sqlite3_array[0];
const char* const** sqlstmts_sqlite3_qschema = &sqlstmts_sqlite3_array[1];
const char* const** sqlstmts_sqlite3_qschema1 = &sqlstmts_sqlite3_array[2];
const char* const** sqlstmts_sqlite3_qschema2 = &sqlstmts_sqlite3_array[3];
const char* const** sqlstmts_sqlite3_default = &sqlstmts_sqlite3_array[4];
const char* const** sqlstmts_sqlite3_schema = &sqlstmts_sqlite3_array[5];
const char* const** sqlstmts_sqlite3_schema1 = &sqlstmts_sqlite3_array[6];
