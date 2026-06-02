#!/bin/bash
# 批量注册学生 — 通过管理员批量导入 API
set -e

BASE_URL="${1:-http://localhost:8080}"
COUNT="${2:-100}"

echo ">>> 1. 管理员登录获取 token ..."
LOGIN_RESP=$(curl -s -X POST "$BASE_URL/api/admin/login" \
  -H "Content-Type: application/json" \
  -d '{"studentNo":"admin","password":"admin123"}')
TOKEN=$(echo "$LOGIN_RESP" | python3 -c "import sys,json; print(json.load(sys.stdin)['data']['token'])" 2>/dev/null)

if [ -z "$TOKEN" ]; then
  echo "登录失败: $LOGIN_RESP"
  exit 1
fi
echo "Token: $TOKEN"

echo ">>> 2. 生成 ${COUNT} 名学生数据 ..."

STUDENTS='['
for i in $(seq 1 $COUNT); do
  # 生成学号，如 20240001
  SNO=$(printf "2024%03d" $i)
  # 随机学院
  COLLEGES=("计算机学院" "电子工程学院" "数学学院" "物理学院" "化学学院" "文学院" "经管学院" "外语学院")
  MAJORS_CS=("软件工程" "计算机科学" "人工智能" "信息安全")
  MAJORS_EE=("通信工程" "电子科学与技术" "微电子")
  MAJORS_MATH=("应用数学" "统计学" "数据科学")
  MAJORS_OTHER=("应用物理" "化学生物" "汉语言文学" "工商管理" "英语")
  GENDERS=("male" "female")

  COLLEGE_IDX=$((RANDOM % ${#COLLEGES[@]}))
  COLLEGE="${COLLEGES[$COLLEGE_IDX]}"

  case $COLLEGE in
    "计算机学院") COL_MAJORS=("${MAJORS_CS[@]}") ;;
    "电子工程学院") COL_MAJORS=("${MAJORS_EE[@]}") ;;
    "数学学院") COL_MAJORS=("${MAJORS_MATH[@]}") ;;
    *) COL_MAJORS=("${MAJORS_OTHER[@]}") ;;
  esac

  MAJOR="${COL_MAJORS[$((RANDOM % ${#COL_MAJORS[@]}))]}"
  GENDER="${GENDERS[$((RANDOM % 2))]}"
  GRADE=$((2021 + RANDOM % 4))

  if [ $i -gt 1 ]; then STUDENTS+=','; fi
  STUDENTS+="{\"studentNo\":\"$SNO\",\"gender\":\"$GENDER\",\"college\":\"$COLLEGE\",\"major\":\"$MAJOR\",\"grade\":\"$GRADE\",\"dormType\":4}"
done
STUDENTS+=']'

echo ">>> 3. 批量导入 ..."
IMPORT_RESP=$(curl -s -X POST "$BASE_URL/api/admin/students/import" \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer $TOKEN" \
  -d "$STUDENTS")

echo "$IMPORT_RESP" | python3 -m json.tool 2>/dev/null || echo "$IMPORT_RESP"

# 验证几个登录
echo ""
echo ">>> 4. 验证：抽查前3个学生登录 ..."
for i in 1 2 3; do
  SNO=$(printf "2024%03d" $i)
  RESP=$(curl -s -X POST "$BASE_URL/api/student/login" \
    -H "Content-Type: application/json" \
    -d "{\"studentNo\":\"$SNO\",\"password\":\"123456\"}")
  TOK=$(echo "$RESP" | python3 -c "import sys,json; d=json.load(sys.stdin); print(d.get('data',{}).get('token','FAIL'))" 2>/dev/null)
  echo "  $SNO -> ${TOK:0:10}..."
done
echo ">>> 完成！"
