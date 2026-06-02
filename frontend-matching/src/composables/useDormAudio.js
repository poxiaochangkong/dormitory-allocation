// 增强的环境音效 — 脚步声 + 开关门 + 窗外环境 + 原有噪音
let audioCtx = null
let noiseNode, acHumNode, gainNode, filterNode
let isPlaying = false
let footstepTimer = 0
let lastFootPos = { x: 0, z: 0 }

export function useDormAudio() {
  const init = () => {
    if (audioCtx) return
    audioCtx = new (window.AudioContext || window.webkitAudioContext)()

    gainNode = audioCtx.createGain()
    gainNode.gain.value = 0.07
    gainNode.connect(audioCtx.destination)

    filterNode = audioCtx.createBiquadFilter()
    filterNode.type = 'lowpass'
    filterNode.frequency.value = 900
    filterNode.connect(gainNode)

    // 棕噪声
    const bufSize = 2 * audioCtx.sampleRate
    const buffer = audioCtx.createBuffer(1, bufSize, audioCtx.sampleRate)
    const data = buffer.getChannelData(0)
    let b0 = 0, b1 = 0, b2 = 0
    for (let i = 0; i < bufSize; i++) {
      const w = Math.random() * 2 - 1
      b0 = 0.99765 * b0 + w * 0.099046
      b1 = 0.963 * b1 + w * 0.2965164
      b2 = 0.570 * b2 + w * 1.052691
      data[i] = (b0 + b1 + b2 + w * 0.1848) * 0.08
    }
    noiseNode = audioCtx.createBufferSource()
    noiseNode.buffer = buffer; noiseNode.loop = true
    noiseNode.connect(filterNode)

    acHumNode = audioCtx.createOscillator()
    acHumNode.type = 'sine'; acHumNode.frequency.value = 50
    const humGain = audioCtx.createGain(); humGain.gain.value = 0.025
    acHumNode.connect(humGain); humGain.connect(filterNode)

    noiseNode.start(); acHumNode.start()
    isPlaying = true
  }

  const playNoise = (freq, dur, vol, type = 'sine') => {
    if (!audioCtx) return
    const o = audioCtx.createOscillator()
    const g = audioCtx.createGain()
    o.type = type; o.frequency.value = freq
    g.gain.setValueAtTime(vol, audioCtx.currentTime)
    g.gain.exponentialRampToValueAtTime(0.001, audioCtx.currentTime + dur)
    o.connect(g); g.connect(audioCtx.destination)
    o.start(); o.stop(audioCtx.currentTime + dur)
  }

  const playFootstep = (pos) => {
    if (!audioCtx || !isPlaying) return
    const now = Date.now()
    const dist = Math.abs(pos.x - lastFootPos.x) + Math.abs(pos.z - lastFootPos.z)
    if (now - footstepTimer < 380) return
    footstepTimer = now; lastFootPos = pos
    // 短促低频脚步声
    playNoise(100 + Math.random() * 80, 0.08, 0.08, 'triangle')
  }

  const playDoor = () => playNoise(250, 0.3, 0.15, 'sawtooth')
  const playFlush = () => {
    playNoise(300, 0.1, 0.1, 'square')
    setTimeout(() => playNoise(180, 0.8, 0.12, 'triangle'), 150)
  }
  const playBird = () => playNoise(1500 + Math.random() * 2000, 0.2, 0.04, 'sine')

  // Random bird every 15-30s
  let birdInterval = null
  const startAmbient = () => {
    if (birdInterval) return
    birdInterval = setInterval(() => { if (Math.random() < 0.4) playBird() }, 15000 + Math.random() * 15000)
  }
  const stopAmbient = () => { clearInterval(birdInterval); birdInterval = null }

  const setVolume = (v) => { if (gainNode) gainNode.gain.value = v * 0.12 }
  const setFilter = (f) => { if (filterNode) filterNode.frequency.value = f }
  const toggleNight = (night) => { setFilter(night ? 350 : 1100); setVolume(night ? 0.25 : 0.55) }
  const playClick = () => playNoise(1200, 0.03, 0.12, 'square')

  const dispose = () => {
    stopAmbient()
    try { noiseNode?.stop() } catch (e) {}
    try { acHumNode?.stop() } catch (e) {}
    audioCtx?.close(); audioCtx = null; isPlaying = false
  }

  return { init, setVolume, toggleNight, playClick, playFootstep, playDoor, playFlush, startAmbient, stopAmbient, dispose }
}
