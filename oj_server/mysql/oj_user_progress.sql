SET NAMES utf8mb4;
SET FOREIGN_KEY_CHECKS = 0;

-- ----------------------------
-- Table structure for oj_user_progress
-- ----------------------------
DROP TABLE IF EXISTS `oj_user_progress`;
CREATE TABLE `oj_user_progress`  (
  `user_id` int UNSIGNED NOT NULL,
  `question_number` int NOT NULL,
  `status` enum('attempted','solved') CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci NOT NULL,
  `updated_at` timestamp NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`user_id`, `question_number`) USING BTREE,
  CONSTRAINT `oj_user_progress_ibfk_1` FOREIGN KEY (`user_id`) REFERENCES `oj_users` (`id`) ON DELETE CASCADE ON UPDATE RESTRICT
) ENGINE = InnoDB CHARACTER SET = utf8mb4 COLLATE = utf8mb4_0900_ai_ci ROW_FORMAT = Dynamic;

SET FOREIGN_KEY_CHECKS = 1;
