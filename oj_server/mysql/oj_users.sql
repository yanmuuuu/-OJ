SET NAMES utf8mb4;
SET FOREIGN_KEY_CHECKS = 0;

-- ----------------------------
-- Table structure for oj_users
-- ----------------------------
DROP TABLE IF EXISTS `oj_users`;
CREATE TABLE `oj_users` (
  `id`          int(10) unsigned NOT NULL AUTO_INCREMENT,
  `username`    varchar(32) NOT NULL COMMENT '登录名',
  `password`    varchar(64) NOT NULL COMMENT '密码哈希(hex)',
  `salt`        varchar(32) NOT NULL COMMENT '盐值',
  `nickname`    varchar(32) DEFAULT '' COMMENT '显示名，可与 username 相同',
  `created_at`  timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE KEY `username` (`username`),
  KEY `idx_username` (`username`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

SET FOREIGN_KEY_CHECKS = 1;