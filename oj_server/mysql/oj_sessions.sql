SET NAMES utf8mb4;
SET FOREIGN_KEY_CHECKS = 0;

-- ----------------------------
-- Table structure for oj_sessions
-- ----------------------------
DROP TABLE IF EXISTS `oj_sessions`;
CREATE TABLE `oj_sessions` (
  `session_id`  varchar(64) NOT NULL COMMENT '随机 token',
  `user_id`     int(10) unsigned NOT NULL,
  `expire_at`   datetime NOT NULL COMMENT '过期时间',
  `created_at`  timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`session_id`),
  KEY `idx_user_id` (`user_id`),
  KEY `idx_expire` (`expire_at`),
  CONSTRAINT `oj_sessions_ibfk_1` FOREIGN KEY (`user_id`) REFERENCES `oj_users` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

SET FOREIGN_KEY_CHECKS = 1;