SET NAMES utf8mb4;
SET FOREIGN_KEY_CHECKS = 0;

-- ----------------------------
-- Table structure for oj_questions
-- ----------------------------
DROP TABLE IF EXISTS `oj_questions`;
CREATE TABLE `oj_questions`  (
  `number` int NOT NULL AUTO_INCREMENT COMMENT '题目编号',
  `title` varchar(128) CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci NOT NULL COMMENT '题目标题',
  `star` varchar(16) CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci NOT NULL COMMENT '题目难度',
  `desc` text CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci NOT NULL COMMENT '题目描述',
  `header` text CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci NOT NULL COMMENT '题目预设',
  `tail` text CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci NOT NULL COMMENT '题目测试用例',
  `cpu_limit` int NOT NULL COMMENT '题目最大运行时间',
  `mem_limit` int NOT NULL COMMENT '题目最大运行空间',
  `author_id` int NOT NULL COMMENT '出题用户 id, NULL 表示官方录入',
  `run_case` text CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci NOT NULL COMMENT '自定义运行用例模板',
  PRIMARY KEY (`number` DESC) USING BTREE,
  INDEX `idx_author_id`(`author_id` ASC) USING BTREE
) ENGINE = InnoDB AUTO_INCREMENT = 6 CHARACTER SET = utf8mb4 COLLATE = utf8mb4_0900_ai_ci ROW_FORMAT = Dynamic;

SET FOREIGN_KEY_CHECKS = 1;
