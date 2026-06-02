<template>
  <router-view></router-view>
</template>

<style>
/* ===== 全局样式 ===== */
html, body, #app {
  margin: 0;
  padding: 0;
  height: 100%;
  width: 100%;
  font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', 'PingFang SC',
    'Hiragino Sans GB', 'Microsoft YaHei', 'Helvetica Neue', Helvetica, Arial,
    sans-serif;
  -webkit-font-smoothing: antialiased;
  -moz-osx-font-smoothing: grayscale;
  color: #303133;
  background: #f0f2f5;
}

/* Element Plus 主题覆盖 */
:root {
  --el-color-primary: #409EFF;
  --el-color-success: #67C23A;
  --el-color-warning: #E6A23C;
  --el-color-danger: #F56C6C;
}

/* 页面通用布局 */
.page-container {
  padding: 20px;
  min-height: 100vh;
  background: #f0f2f5;
}

/* 通用卡片增强 */
.el-card {
  border-radius: 12px !important;
  box-shadow: 0 2px 12px rgba(0, 0, 0, 0.06) !important;
}

.el-card__header {
  border-bottom: 1px solid #ebeef5;
  padding: 16px 24px !important;
}

/* 页面标题 */
.page-title {
  font-size: 22px;
  font-weight: 700;
  color: #303133;
  margin: 0;
}

/* 统计数字卡片 */
.stat-card {
  text-align: center;
  padding: 10px 0;
}
.stat-card .stat-number {
  font-size: 32px;
  font-weight: 700;
  color: #409EFF;
}
.stat-card .stat-label {
  font-size: 14px;
  color: #909399;
  margin-top: 4px;
}

/* 按钮组间距 */
.header-actions {
  display: flex;
  gap: 8px;
  flex-wrap: wrap;
}

/* 表格通用 */
.el-table {
  border-radius: 8px;
  overflow: hidden;
}

/* 对话框 */
.el-dialog {
  border-radius: 12px !important;
}

/* 动画 */
@keyframes fadeInUp {
  from { opacity: 0; transform: translateY(20px); }
  to { opacity: 1; transform: translateY(0); }
}
.fade-in { animation: fadeInUp 0.4s ease; }
</style>