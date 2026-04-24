const { app, BrowserWindow, shell } = require('electron')
const { spawn } = require('child_process')
const path = require('path')
const http = require('http')

let mainWindow
let flaskProcess

// Find the Flask backend executable
function getFlaskPath() {
  if (app.isPackaged) {
    return path.join(process.resourcesPath, 'backend', 'IPFinder.exe')
  } else {
    return null
  }
}

// Wait for Flask to be ready by polling /api/health
function waitForFlask(retries = 20) {
  return new Promise((resolve, reject) => {
    const check = (remaining) => {
      if (remaining <= 0) return reject(new Error('Flask server did not start'))
      http.get('http://localhost:5000/api/health', (res) => {
        if (res.statusCode === 200) resolve()
        else setTimeout(() => check(remaining - 1), 500)
      }).on('error', () => setTimeout(() => check(remaining - 1), 500))
    }
    check(retries)
  })
}

function startFlask() {
  const flaskPath = getFlaskPath()

  if (flaskPath) {
    flaskProcess = spawn(flaskPath, [], {
      cwd: path.dirname(flaskPath),
      windowsHide: true
    })

    flaskProcess.on('error', (err) => {
      console.error('Failed to start Flask:', err)
    })
  } else {
    const projectRoot = path.join(__dirname, '..', '..')
    flaskProcess = spawn('python', ['server.py'], {
      cwd: projectRoot,
      windowsHide: true
    })
  }
}

function createWindow() {
  mainWindow = new BrowserWindow({
    width: 1100,
    height: 750,
    minWidth: 800,
    minHeight: 600,
    title: 'IP Finder',
    webPreferences: {
      nodeIntegration: false,
      contextIsolation: true,
    },
    autoHideMenuBar: true,
    backgroundColor: '#0d0d0d',
  })

  // Load the React app from Flask
  mainWindow.loadURL('http://localhost:5000')

  mainWindow.webContents.setWindowOpenHandler(({ url }) => {
    shell.openExternal(url)
    return { action: 'deny' }
  })

  mainWindow.on('closed', () => {
    mainWindow = null
  })
}

app.whenReady().then(async () => {
  startFlask()

  try {
    await waitForFlask()
    createWindow()
  } catch (err) {
    console.error('Could not connect to Flask backend:', err)
    app.quit()
  }
})

app.on('window-all-closed', () => {
  // Kill Flask when app closes
  if (flaskProcess) {
    flaskProcess.kill()
  }
  app.quit()
})

app.on('activate', () => {
  if (BrowserWindow.getAllWindows().length === 0) createWindow()
})