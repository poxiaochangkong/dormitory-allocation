// 标准四人寝场景 + 第一人称 WASD 漫游 + 热点问答
import * as THREE from 'three'

let scene, camera, renderer, clock
let hotspotMeshes = []
let onHotspotClick = null
let onMoveCallback = null
let animationId = null
let ambientLight, pointLight, dirLight

// ===== 第一人称移动状态 =====
let moveForward = false, moveBackward = false, moveLeft = false, moveRight = false
let pointerLocked = false
const euler = new THREE.Euler(0, 0, 0, 'YXZ')   // yaw-pitch, rotation order
const PI_2 = Math.PI / 2
const velocity = new THREE.Vector3()
const direction = new THREE.Vector3()
const MOVE_SPEED = 12.0          // m/s (快节奏移动)
const DAMPING = 10.0

// ===== 准星 / 热点悬停 =====
let hoveredHotspot = null
const raycaster = new THREE.Raycaster()
raycaster.far = 4.0               // 热点交互最大距离

// ===== 房间参数 =====
const RW = 4.0    // 房间宽 (X)
const RD = 7.0    // 房间深 (Z, 不含阳台)
const RH = 3.0    // 层高
const HW = RW / 2
const HD = RD / 2

// ===== 碰撞边界 (略小于视觉墙体) =====
const BOUNDS = { xMin: -1.85, xMax: 1.85, zMin: -5.0, zMax: 3.2 }

// ===== 材质工厂 =====
const WL = () => new THREE.MeshStandardMaterial({ color: 0xf8f3e8, roughness: 0.8 })
const FL = () => new THREE.MeshStandardMaterial({ color: 0xc8b090, roughness: 0.5 })
const WD = () => new THREE.MeshStandardMaterial({ color: 0xb8956e, roughness: 0.5 })
const MT = () => new THREE.MeshStandardMaterial({ color: 0xa0a0a0, roughness: 0.25, metalness: 0.8 })
const WH = () => new THREE.MeshStandardMaterial({ color: 0xf8f8f8, roughness: 0.3 })
const GL = () => new THREE.MeshStandardMaterial({ color: 0xa0c8e0, roughness: 0.05, metalness: 0.1, opacity: 0.45, transparent: true })
const TILE = () => new THREE.MeshStandardMaterial({ color: 0xe0dbd0, roughness: 0.35 })
const BTILE = () => new THREE.MeshStandardMaterial({ color: 0xd5cfc4, roughness: 0.4 })
const BED = (c) => new THREE.MeshStandardMaterial({ color: c, roughness: 0.6 })
const BK = (c) => new THREE.MeshStandardMaterial({ color: c, roughness: 0.7 })
const RED = () => new THREE.MeshStandardMaterial({ color: 0xf56c6c, emissive: 0xf56c6c, emissiveIntensity: 0.6 })

// ===== 几何辅助 =====
function box(w, h, d, mat, x, y, z) {
  const g = new THREE.BoxGeometry(w, h, d)
  const m = new THREE.Mesh(g, typeof mat === 'function' ? mat() : mat)
  m.position.set(x, y, z); return m
}
function cyl(r1, r2, h, s, mat, x, y, z) {
  const g = new THREE.CylinderGeometry(r1, r2, h, s)
  const m = new THREE.Mesh(g, typeof mat === 'function' ? mat() : mat)
  m.position.set(x, y, z); return m
}
function pnl(w, h, mat, x, y, z) {
  const g = new THREE.PlaneGeometry(w, h)
  const m = new THREE.Mesh(g, typeof mat === 'function' ? mat() : mat)
  m.position.set(x, y, z); return m
}
function hs(x, y, z, key) {
  const g = new THREE.SphereGeometry(0.09, 12, 12)
  const m = new THREE.Mesh(g, RED())
  m.position.set(x, y, z); m.userData = { isHotspot: true, key }; m.renderOrder = 999; m.material.depthWrite = false
  return m
}

// ===== 上床下桌 ×1 =====
function bedDesk(bx, bz, quiltColor, side) {
  const g = new THREE.Group()
  const dx = side === 'L' ? bx - 0.42 : bx + 0.42

  // 钢架立柱
  const corners = [[bx-0.4,bz-0.9],[bx+0.4,bz-0.9],[bx-0.4,bz+0.9],[bx+0.4,bz+0.9]]
  corners.forEach(([fx,fz]) => { g.add(box(0.04, 2.6, 0.04, MT, fx, 1.3, fz)) })
  g.add(box(0.9, 0.03, 0.04, MT, bx, 1.85, bz))
  g.add(box(0.9, 0.03, 0.04, MT, bx, 0.85, bz))

  // 床板 + 床垫
  g.add(box(0.88, 0.04, 1.95, WD, bx, 1.8, bz))
  g.add(box(0.82, 0.1, 1.85, WH, bx, 1.87, bz))
  // 被子
  g.add(box(0.78, 0.13, 0.7, BED(quiltColor), bx, 1.96, bz + 0.42))
  // 枕头
  g.add(box(0.32, 0.08, 0.55, WH, bx, 1.91, bz - 0.63))
  // 床护栏
  g.add(box(0.03, 0.25, 1.85, MT, bx+0.42, 1.98, bz))

  // 爬梯
  const lx = bx + 0.35, lz = bz + 0.88
  for (let i=0; i<4; i++) g.add(box(0.03, 0.04, 0.4, MT, lx, 0.45+i*0.48, lz))
  g.add(box(0.04, 2.1, 0.04, MT, lx, 1.05, lz))

  // 书桌
  g.add(box(0.88, 0.03, 0.55, WD, dx, 0.78, bz))
  g.add(box(0.02, 0.45, 0.32, WD, dx+0.36, 1.0, bz+0.12))
  g.add(box(0.75, 0.02, 0.32, WD, dx, 1.25, bz+0.12))

  // 椅子
  g.add(box(0.34, 0.03, 0.34, MT, dx, 0.45, bz+0.55))
  g.add(cyl(0.025, 0.025, 0.4, 8, MT, dx, 0.25, bz+0.55))

  // 桌面物品(整洁)
  const clean = new THREE.Group(); clean.name = 'cleanItems'
  clean.add(box(0.15, 0.2, 0.05, BK(0x4488cc), dx+0.15, 0.87, bz-0.06))
  clean.add(box(0.14, 0.07, 0.04, BK(0xcc4444), dx+0.17, 0.94, bz-0.04))
  clean.add(box(0.28, 0.02, 0.18, MT, dx-0.15, 0.81, bz))
  clean.add(cyl(0.04, 0.04, 0.11, 10, WH, dx-0.2, 0.84, bz+0.12))
  g.add(clean)

  // 桌面物品(杂乱) — 初始隐藏
  const messy = new THREE.Group(); messy.name = 'messyItems'; messy.visible = false
  messy.add(box(0.15, 0.05, 0.04, BK(0x4488cc), dx+0.1, 0.81, bz-0.1))
  messy.add(box(0.13, 0.04, 0.03, BK(0xcc4444), dx-0.2, 0.80, bz+0.05))
  messy.add(box(0.16, 0.18, 0.04, BK(0xf5a623), dx+0.2, 0.86, bz+0.1))
  messy.add(box(0.1, 0.07, 0.05, WH, dx-0.05, 0.80, bz-0.15))
  messy.add(cyl(0.035, 0.035, 0.13, 10, MT, dx-0.2, 0.83, bz-0.12))
  g.add(messy)

  return g
}

// ===== 阳台 =====
function balcony() {
  const g = new THREE.Group()
  const bz = -HD - 1.0   // 阳台中心 Z
  const bd = 1.2          // 阳台深度

  // 地板
  g.add(box(RW, 0.04, bd, FL, 0, -0.02, bz))
  // 推拉门框+玻璃
  g.add(box(2.2, 2.5, 0.05, WD, 0, 1.25, -HD))
  g.add(pnl(2.2, 2.5, GL, 0, 1.25, -HD+0.01))
  // 护栏
  for (let y=0.3; y<=1.1; y+=0.35) g.add(box(0.03, 0.04, bd+0.1, MT, -HW+0.02, y, bz))
  // 右侧护栏被卫生间墙壁替代，保留顶部横杆
  for (let y=0.3; y<=1.1; y+=0.35) g.add(box(0.03, 0.04, bd+0.1, MT, 0.55, y, bz))
  g.add(box(RW-0.04, 0.04, 0.03, MT, 0, 1.1, bz-bd/2-0.02))
  for (let x=-HW+0.1; x<=0.3; x+=0.45) g.add(box(0.04, 1.15, 0.04, MT, x, 0.7, bz-bd/2-0.02))
  // 晾衣杆(左半段，右侧被卫生间占据)
  g.add(cyl(0.015, 0.015, 1.8, 8, MT, -0.5, 2.1, bz-0.2))
  // 衣服
  ;[0xe74c3c,0x3498db].forEach((c,i) => g.add(box(0.32, 0.5, 0.02, BED(c), -1.0+i*0.8, 2.05, bz-0.2)))
  return g
}

// ===== 卫生间 (右后角, 阳台区域, 全封闭) =====
function bathroom() {
  const g = new THREE.Group()
  const bx = 1.35        // 卫生间中心 X (紧贴右墙)
  const bz = -4.3        // 卫生间中心 Z (阳台区域)
  const bw = 1.3         // 宽 (X)
  const bd = 1.8         // 深 (Z)
  const bh = 2.7         // 高
  const hw = bw / 2, hd = bd / 2

  // -- 地板 --
  g.add(box(bw, 0.04, bd, BTILE, bx, 0.02, bz))

  // -- 天花板 --
  g.add(box(bw, 0.04, bd, BTILE, bx, bh-0.02, bz))

  // -- 左墙 (分隔走廊) --
  g.add(box(0.06, bh, bd, BTILE, bx - hw, bh/2, bz))

  // -- 右墙 (建筑外墙, 与阳台护栏重叠) --
  g.add(box(0.06, bh, bd, BTILE, bx + hw, bh/2, bz))

  // -- 后墙 (建筑外墙) --
  g.add(box(bw, bh, 0.06, BTILE, bx, bh/2, bz - hd))

  // -- 前墙 (带门洞, 面朝阳台) --
  const doorW = 0.65, doorH = 2.05, doorCx = bx + 0.05  // 门偏左
  const wallH = bh - doorH
  // 左侧墙垛
  const leftW = doorCx - hw - 0.03
  if (leftW > 0.02) g.add(box(leftW, bh, 0.06, BTILE, doorCx - leftW/2 - doorW/2 - 0.01, bh/2, bz + hd))
  // 右侧墙垛
  const rightW = bx + hw - (doorCx + doorW/2)
  if (rightW > 0.02) g.add(box(rightW, bh, 0.06, BTILE, doorCx + doorW/2 + rightW/2, bh/2, bz + hd))
  // 门楣
  g.add(box(doorW + 0.02, wallH, 0.06, BTILE, doorCx, doorH + wallH/2, bz + hd))

  // -- 蹲便器 --
  g.add(box(0.35, 0.04, 0.5, WH, bx + 0.05, 0.04, bz - 0.15))
  // 冲水箱
  g.add(box(0.32, 0.4, 0.14, WH, bx + 0.05, 0.35, bz - hd + 0.07))

  // -- 淋浴区 (后墙) --
  g.add(cyl(0.012, 0.012, 0.4, 8, MT, bx + 0.28, 2.4, bz - hd + 0.06))
  g.add(cyl(0.07, 0.07, 0.03, 12, MT, bx + 0.28, 2.6, bz - hd + 0.06))
  // 淋浴地漏区
  g.add(box(0.5, 0.01, 0.5, new THREE.MeshStandardMaterial({ color: 0xc8c0b8, roughness: 0.3 }), bx + 0.25, 0.025, bz - 0.55))

  // -- 洗手台 (靠左墙) --
  g.add(box(0.5, 0.04, 0.35, WH, bx - 0.2, 0.85, bz + hd - 0.22))
  // 台下盆
  g.add(cyl(0.14, 0.12, 0.1, 16, WH, bx - 0.2, 0.78, bz + hd - 0.22))
  // 水龙头
  g.add(cyl(0.02, 0.02, 0.18, 10, MT, bx - 0.2, 0.95, bz + hd - 0.4))

  // -- 镜子 (前墙内侧, 洗手台上方) --
  g.add(pnl(0.42, 0.5, new THREE.MeshStandardMaterial({ color: 0xddeeff, roughness: 0.02, metalness: 0.9 }), bx - 0.2, 1.35, bz + hd - 0.02))

  // -- 毛巾架 --
  g.add(cyl(0.012, 0.012, 0.45, 8, MT, bx - 0.25, 1.55, bz + hd - 0.25))

  return g
}

// ===== 完整宿舍 =====
function buildDorm() {
  const root = new THREE.Group()

  // 地板+天花板
  root.add(box(RW, 0.04, RD, FL, 0, -0.02, 0))
  root.add(box(RW, 0.04, RD, new THREE.MeshStandardMaterial({ color: 0xfaf8f2, roughness: 0.85 }), 0, RH, 0))

  // 墙体
  root.add(box(RW, RH, 0.06, WL, 0, RH/2, -HD))  // 后
  root.add(box(RW, RH, 0.06, WL, 0, RH/2, HD))    // 前
  root.add(box(0.06, RH, RD, WL, -HW, RH/2, 0))   // 左
  root.add(box(0.06, RH, RD, WL, HW, RH/2, 0))    // 右

  // 门(前墙左侧)
  root.add(box(0.9, 2.15, 0.03, WD, -1.0, 1.08, HD-0.01))
  root.add(cyl(0.03, 0.03, 0.03, 8, new THREE.MeshStandardMaterial({ color: 0xffd700, roughness: 0.2, metalness: 0.9 }), -0.7, 1.08, HD+0.01))

  // 窗户(后墙)
  const wx = 0.5, wy = 1.7, ww = 1.8, wh = 1.4, wz = -HD
  root.add(box(ww+0.1, wh+0.1, 0.05, WD, wx, wy, wz+0.02))
  root.add(pnl(ww, wh, GL, wx, wy, wz+0.05))
  root.add(box(ww+0.03, 0.03, 0.02, WD, wx, wy, wz+0.05))
  root.add(box(0.03, wh+0.03, 0.02, WD, wx, wy, wz+0.05))

  // 窗帘
  root.add(pnl(0.4, wh+0.25, new THREE.MeshStandardMaterial({ color: 0xd4b896, roughness: 0.5, side: THREE.DoubleSide }), wx-ww/2-0.22, wy, wz+0.18))

  // 空调(后墙上部)
  const ac = new THREE.Group(); ac.position.set(wx+1.2, RH-0.5, wz+0.04)
  ac.add(box(0.85, 0.25, 0.15, new THREE.MeshStandardMaterial({ color: 0xf2f2f2, roughness: 0.2, metalness: 0.5 }), 0, 0, 0))
  for (let gi=0; gi<6; gi++) ac.add(box(0.7, 0.012, 0.015, MT, 0, -0.07+gi*0.04, 0.08))
  root.add(ac)

  // 灯×2
  root.add(box(0.9, 0.04, 0.22, new THREE.MeshStandardMaterial({ color: 0xfffff0, emissive: 0xffffcc, emissiveIntensity: 0.3 }), 0, RH-0.02, -1.2))
  root.add(box(0.9, 0.04, 0.22, new THREE.MeshStandardMaterial({ color: 0xfffff0, emissive: 0xffffcc, emissiveIntensity: 0.3 }), 0, RH-0.02, 1.2))

  // 垃圾桶(进门右侧)
  root.add(cyl(0.14, 0.12, 0.4, 14, MT, HW-0.3, 0.2, 2.8))

  // 墙上装饰
  root.add(pnl(0.6, 0.8, new THREE.MeshStandardMaterial({ color: 0xe8d44d, roughness: 0.85 }), -HW+0.01, 1.9, 0))
  root.add(pnl(0.45, 0.3, new THREE.MeshStandardMaterial({ color: 0x88ccee, roughness: 0.85 }), HW-0.01, 1.5, -1.5))

  // 4组上床下桌
  root.add(bedDesk(-1.3, 2.0, 0x4a90d9, 'L'))
  root.add(bedDesk(-1.3, -0.1, 0x50c878, 'L'))
  root.add(bedDesk(1.3, 2.0, 0xe8915c, 'R'))
  root.add(bedDesk(1.3, -0.1, 0x9b7ec4, 'R'))

  // 阳台+卫生间
  root.add(balcony())
  root.add(bathroom())

  return root
}

// ===== 第一人称控制 =====
// 存储事件处理器引用以便销毁时移除
let _eCanvas = null, _resizeHandler = null
let _onClick = null, _onPtrChange = null, _onMouse = null, _onKeyDn = null, _onKeyUp = null, _onWheel = null

function teardownFirstPerson() {
  if (_onClick && _eCanvas) { _eCanvas.removeEventListener('click', _onClick); _eCanvas.removeEventListener('wheel', _onWheel) }
  if (_onPtrChange) document.removeEventListener('pointerlockchange', _onPtrChange)
  if (_onMouse)      document.removeEventListener('mousemove', _onMouse)
  if (_onKeyDn)      document.removeEventListener('keydown', _onKeyDn)
  if (_onKeyUp)      document.removeEventListener('keyup', _onKeyUp)
  if (_resizeHandler) window.removeEventListener('resize', _resizeHandler)
  _onClick = _onPtrChange = _onMouse = _onKeyDn = _onKeyUp = _onWheel = _resizeHandler = null
  _eCanvas = null
  document.exitPointerLock()
}

function setupFirstPerson(canvas) {
  // 先清理旧监听器 (防止重置场景时累积)
  teardownFirstPerson()
  _eCanvas = canvas

  // 点击画布 → 锁定指针 / 交互热点
  _onClick = () => {
    if (!pointerLocked) {
      canvas.requestPointerLock()
    } else if (hoveredHotspot && onHotspotClick) {
      onHotspotClick(hoveredHotspot.userData.key)
    }
  }
  canvas.addEventListener('click', _onClick)

  // 指针锁定状态变化
  _onPtrChange = () => { pointerLocked = document.pointerLockElement === canvas }
  document.addEventListener('pointerlockchange', _onPtrChange)

  // 鼠标视角
  _onMouse = (e) => {
    if (!pointerLocked) return
    euler.y -= e.movementX * 0.0022
    euler.x -= e.movementY * 0.0022
    euler.x = Math.max(-PI_2 * 0.85, Math.min(PI_2 * 0.85, euler.x))
  }
  document.addEventListener('mousemove', _onMouse)

  // 键盘
  _onKeyDn = (e) => {
    if (!pointerLocked) return
    switch (e.code) {
      case 'KeyW': case 'ArrowUp':    moveForward = true; break
      case 'KeyS': case 'ArrowDown':  moveBackward = true; break
      case 'KeyA': case 'ArrowLeft':  moveLeft = true; break
      case 'KeyD': case 'ArrowRight': moveRight = true; break
      case 'KeyE':
        if (hoveredHotspot && onHotspotClick) onHotspotClick(hoveredHotspot.userData.key)
        break
    }
  }
  _onKeyUp = (e) => {
    switch (e.code) {
      case 'KeyW': case 'ArrowUp':    moveForward = false; break
      case 'KeyS': case 'ArrowDown':  moveBackward = false; break
      case 'KeyA': case 'ArrowLeft':  moveLeft = false; break
      case 'KeyD': case 'ArrowRight': moveRight = false; break
    }
  }
  document.addEventListener('keydown', _onKeyDn)
  document.addEventListener('keyup', _onKeyUp)

  // 滚轮无操作
  _onWheel = (e) => e.preventDefault()
  canvas.addEventListener('wheel', _onWheel, { passive: false })
}

// ===== 碰撞检测 =====
function clampPosition(pos) {
  pos.x = Math.max(BOUNDS.xMin, Math.min(BOUNDS.xMax, pos.x))
  pos.z = Math.max(BOUNDS.zMin, Math.min(BOUNDS.zMax, pos.z))
  return pos
}

// ===== 初始化场景 =====
export function initScene(canvas, callbacks) {
  onHotspotClick = callbacks?.onHotspot || null
  onMoveCallback = callbacks?.onMove || null

  scene = new THREE.Scene()
  scene.background = new THREE.Color(0x8899aa)

  const w = canvas.clientWidth || 800
  const h = canvas.clientHeight || 500
  camera = new THREE.PerspectiveCamera(65, w / h, 0.3, 25)
  // 初始位置: 站在门口附近，面朝房间深处
  camera.position.set(0, 1.65, 2.8)
  euler.set(0, Math.PI, 0, 'YXZ')  // 面朝 -Z (房间后部)

  renderer = new THREE.WebGLRenderer({ canvas, antialias: true })
  renderer.setSize(w, h, false)
  renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2))
  renderer.shadowMap.enabled = true
  renderer.shadowMap.type = THREE.PCFSoftShadowMap

  // 灯光
  ambientLight = new THREE.AmbientLight(0xffffff, 1.2)
  scene.add(ambientLight)
  dirLight = new THREE.DirectionalLight(0xffffff, 2.5)
  dirLight.position.set(3, 7, 4)
  dirLight.castShadow = true
  dirLight.shadow.mapSize.set(512, 512)
  dirLight.shadow.camera.near = 0.5; dirLight.shadow.camera.far = 20
  dirLight.shadow.camera.left = -5; dirLight.shadow.camera.right = 5
  dirLight.shadow.camera.top = 5; dirLight.shadow.camera.bottom = -5
  scene.add(dirLight)
  pointLight = new THREE.PointLight(0xfff5e6, 1.5, 8)
  pointLight.position.set(0, 2.5, 0); scene.add(pointLight)

  // 构建宿舍
  const dorm = buildDorm()
  scene.add(dorm)

  // 9个热点 — 放置在场景各处
  hotspotMeshes = [
    hs(1.7, RH-0.35, -HD+0.12, 'ac'),       // 空调 (后墙上部)
    hs(-1.3, 1.3, 2.0, 'bed'),               // 床 (左侧前)
    hs(-1.7, 1.05, -0.1, 'desk'),            // 书桌 (左侧后)
    hs(-1.3, 0.65, 2.0, 'privacy'),          // 私人物品 (床下区域)
    hs(0.5, 1.85, -HD+0.15, 'curtain'),      // 窗帘 (窗户旁)
    hs(HW-0.3, 0.45, 2.8, 'cleanDuty'),      // 值日/垃圾桶 (进门右)
    hs(1.1, 1.15, -4.1, 'wash'),             // 洗漱 (卫生间洗手台前)
    hs(0, 1.55, 0.3, 'noise'),               // 噪声 (房间中央)
    hs(-0.2, 1.35, -4.5, 'balcony'),         // 阳台潮湿 (阳台中央)
  ]
  hotspotMeshes.forEach(h => scene.add(h))

  // 第一人称控制
  setupFirstPerson(canvas)

  // 窗口大小适配
  _resizeHandler = () => {
    const cw = canvas.clientWidth || 800, ch = canvas.clientHeight || 500
    camera.aspect = cw / ch; camera.updateProjectionMatrix()
    renderer.setSize(cw, ch, false)
  }
  window.addEventListener('resize', _resizeHandler)

  clock = new THREE.Clock()
  animate()
}

// ===== 主循环 =====
function animate() {
  animationId = requestAnimationFrame(animate)

  const delta = Math.min(clock.getDelta(), 0.1)  // 防止大帧跳跃

  // ---- 移动 ----
  if (pointerLocked) {
    // 计算移动方向 (基于相机朝向)
    direction.set(0, 0, 0)
    if (moveForward)  direction.z -= 1
    if (moveBackward) direction.z += 1
    if (moveLeft)     direction.x -= 1
    if (moveRight)    direction.x += 1

    if (direction.lengthSq() > 0) {
      direction.normalize()

      // 将局部方向转换为世界方向 (仅 Y 轴旋转)
      const speed = MOVE_SPEED * delta
      const sinY = Math.sin(euler.y), cosY = Math.cos(euler.y)

      velocity.x += (direction.x * cosY + direction.z * sinY) * speed
      velocity.z += (direction.x * -sinY + direction.z * cosY) * speed
    }

    // 阻尼衰减
    velocity.x *= Math.exp(-DAMPING * delta)
    velocity.z *= Math.exp(-DAMPING * delta)

    // 应用速度
    camera.position.x += velocity.x * delta
    camera.position.z += velocity.z * delta
    clampPosition(camera.position)

    // 更新相机旋转
    camera.quaternion.setFromEuler(euler)

    // 脚步声回调
    if (onMoveCallback && (Math.abs(velocity.x) > 0.03 || Math.abs(velocity.z) > 0.03)) {
      onMoveCallback(camera.position)
    }
  }

  // ---- 准星检测热点 ----
  raycaster.setFromCamera(new THREE.Vector2(0, 0), camera)
  const hits = raycaster.intersectObjects(hotspotMeshes)
  const newHovered = hits.length > 0 ? hits[0].object : null

  if (hoveredHotspot !== newHovered) {
    // 还原旧悬停
    if (hoveredHotspot) {
      hoveredHotspot.material.emissive.set(0xf56c6c)
      hoveredHotspot.material.emissiveIntensity = 0.6
    }
    // 高亮新悬停
    if (newHovered) {
      newHovered.material.emissive.set(0xffff00)
      newHovered.material.emissiveIntensity = 1.2
    }
    hoveredHotspot = newHovered
  }

  // ---- 热点脉冲动画 ----
  hotspotMeshes.forEach((h, i) => {
    const base = h === hoveredHotspot ? 1.35 : 1.0
    h.scale.setScalar(base + Math.sin(Date.now() * 0.005 + i) * 0.18)
  })

  renderer.render(scene, camera)
}

// ===== 工具导出 =====
export function toggleNight(night) {
  if (!scene) return
  if (night) {
    scene.background = new THREE.Color(0x1a1a3e)
    ambientLight.intensity = 0.25; pointLight.intensity = 0.35
    pointLight.color.set(0x8899cc); dirLight.intensity = 0.3
  } else {
    scene.background = new THREE.Color(0x8899aa)
    ambientLight.intensity = 1.2; pointLight.intensity = 1.5
    pointLight.color.set(0xfff5e6); dirLight.intensity = 2.5
  }
}

export function setDeskOrganization(messy) {
  if (!scene) return
  scene.traverse(obj => {
    if (obj.name === 'cleanItems') obj.visible = !messy
    if (obj.name === 'messyItems') obj.visible = messy
  })
}

export function isPointerLocked() { return pointerLocked }
export function exitPointerLock() { document.exitPointerLock() }
export function getCameraPos() { return camera ? camera.position.clone() : null }

// 预设场景传送点
const TELEPORT_SPOTS = {
  center:   { pos: [0, 1.65, 2.5],    look: [0, Math.PI, 0] },              // 房间中央(入口)
  bathroom: { pos: [1.15, 1.65, -3.9], look: [0, -Math.PI * 0.42, 0] },     // 卫生间内,朝向洗手台
  balcony:  { pos: [-0.3, 1.65, -4.4], look: [-0.15, Math.PI, 0] },          // 阳台,朝向窗外
  dorm:     { pos: [-0.8, 1.65, 1.5],  look: [0, -Math.PI * 0.55, 0] },      // 床位区域
}

export function teleportTo(spotName) {
  if (!camera) return
  const spot = TELEPORT_SPOTS[spotName]
  if (!spot) return
  camera.position.set(...spot.pos)
  euler.set(...spot.look)
  camera.quaternion.setFromEuler(euler)
  velocity.set(0, 0, 0)
}

export function disposeScene() {
  teardownFirstPerson()
  if (animationId) cancelAnimationFrame(animationId)
  if (renderer) renderer.dispose()
  hotspotMeshes = []
  hoveredHotspot = null
  scene = camera = renderer = null
  moveForward = moveBackward = moveLeft = moveRight = false
  pointerLocked = false
}
