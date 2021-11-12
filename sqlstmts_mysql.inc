const char* const sqlstmts_mysql_qschema_[] = {
  "SELECT count(name) FROM sqlite_master WHERE type='table' AND name='properties'",

  NULL };

const char* const sqlstmts_mysql_schema_[] = {
  "CREATE TABLE properties (\n"
  "  propertyName VARCHAR(32),\n"
  "  propertyValue VARCHAR(255),\n"
  "  PRIMARY KEY ( propertyName )\n"
  ");\n"
  "INSERT INTO properties VALUES ( 'version', 4 );\n"
  "CREATE TABLE policy (\n"
  "  id INT NOT NULL,\n"
  "  name TEXT NOT NULL,\n"
  "  PRIMARY KEY ( id ),\n"
  "  UNIQUE KEY ( name )\n"
  ");\n"
  "CREATE TABLE zone (\n"
  "  id INT NOT NULL,\n"
  "  name TEXT NOT NULL,\n"
  "  revision INT NOT NULL default 1,\n"
  "  policy INT NOT NULL,\n"
  "  parent INT DEFAULT NULL,\n"
  "  PRIMARY KEY ( id ),\n"
  "  UNIQUE KEY ( name )\n"
  ");\n"
  "INSERT INTO policy VALUES ( 0, \"default\" );\n"
  "INSERT INTO policy VALUES ( 1, \"experimental\" );\n"
  "INSERT INTO zone VALUES ( 0, \"example.com\", 1, 0, NULL );\n"
  "INSERT INTO zone VALUES ( 1, \"example.org\", 1, 0, NULL );\n"
  "INSERT INTO zone VALUES ( 2, \"opendnssec.org\", 1, 1, NULL );\n"
  "INSERT INTO zone VALUES ( 3, \"opendnssec.com\", 1, 1, 2 )",

  NULL };

const char* const sqlstmts_mysql_qschema1_[] = {
  "SELECT count(propertyName) FROM properties WHERE propertyName='version' AND propertyValue >= 4",

  NULL };

const char* const sqlstmts_mysql_schema1_[] = {
  "UPDATE properties SET propertyValue = 4 WHERE propertyName = 'version'",

  NULL };

const char* const sqlstmts_mysql_default_[] = {
  "SELECT id, name FROM policy ORDER BY name",

  "SELECT id, revision, name, policy, policy, parent, parent FROM zone ORDER BY name",

  "UPDATE policy SET name = ? WHERE id = ?",

  "DELETE FROM policy WHERE id = ?",

  "INSERT INTO zone ( id, revision, name, policy, parent ) VALUES ( ?, ?, ?, ?, ? )",

  "DELETE FROM zone WHERE id = ? AND revision = ?",

  NULL };

const char* const sqlstmts_mysql_probe_[] = {
  "SELECT 1",

  NULL };

const char* const* sqlstmts_mysql_array[] = {
  sqlstmts_mysql_qschema_,
  sqlstmts_mysql_qschema1_,
  sqlstmts_mysql_default_,
  sqlstmts_mysql_probe_,
  sqlstmts_mysql_schema_,
  sqlstmts_mysql_schema1_,
  NULL 
};

const char* const** sqlstmts_mysql_qschema = &sqlstmts_mysql_array[0];
const char* const** sqlstmts_mysql_qschema1 = &sqlstmts_mysql_array[1];
const char* const** sqlstmts_mysql_default = &sqlstmts_mysql_array[2];
const char* const** sqlstmts_mysql_probe = &sqlstmts_mysql_array[3];
const char* const** sqlstmts_mysql_schema = &sqlstmts_mysql_array[4];
const char* const** sqlstmts_mysql_schema1 = &sqlstmts_mysql_array[5];
