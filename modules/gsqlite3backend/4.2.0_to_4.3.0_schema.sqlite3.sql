BEGIN TRANSACTION;
  CREATE TEMPORARY TABLE cryptokeys_backup (
    id                     INTEGER PRIMARY KEY,
    domain_id              INT NOT NULL,
    flags                  INT NOT NULL,
    active                 BOOL,
    content                TEXT
  );

  INSERT INTO cryptokeys_backup SELECT id, domain_id, flags, active, content FROM cryptokeys;
  DROP TABLE cryptokeys;

  CREATE TABLE cryptokeys (
    id                     INTEGER PRIMARY KEY,
    domain_id              INT NOT NULL,
    flags                  INT NOT NULL,
    active                 BOOL,
    publish                BOOL,
    content                TEXT,
    FOREIGN KEY(domain_id) REFERENCES domains(id) ON DELETE CASCADE ON UPDATE CASCADE
  );

  CREATE INDEX domainidindex ON cryptokeys(domain_id);

  INSERT INTO cryptokeys SELECT id, domain_id, flags, active, 1, content FROM cryptokeys_backup;
  UPDATE cryptokeys SET publish=0 WHERE active=0;
  DROP TABLE cryptokeys_backup;
COMMIT;
