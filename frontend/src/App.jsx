import { useState, useEffect, useRef } from "react"
import "./App.css"

const API_BASE = "http://localhost:5000"

export default function App() {
  const [path, setPath] = useState("")
  const [scanId, setScanId] = useState(null)
  const [status, setStatus] = useState("idle") // idle | running | complete | error
  const [results, setResults] = useState(null)
  const [log, setLog] = useState([])
  const [currentFile, setCurrentFile] = useState("")
  const [parsedCount, setParsedCount] = useState(0)
  const [error, setError] = useState("")
  const logEndRef = useRef(null)
  const pollRef = useRef(null)

  // Auto-scroll log
  useEffect(() => {
    logEndRef.current?.scrollIntoView({ behavior: "smooth" })
  }, [log])

  // Poll for scan progress
  useEffect(() => {
    if (!scanId) return
    pollRef.current = setInterval(async () => {
      try {
        const res = await fetch(`${API_BASE}/api/scan/${scanId}`)
        const data = await res.json()
        setLog(data.log || [])
        setCurrentFile(data.current_file || "")
        setParsedCount(data.parsed_files || 0)

        if (data.status === "complete") {
          clearInterval(pollRef.current)
          setStatus("complete")
          setResults(data)
        }
      } catch (e) {
        clearInterval(pollRef.current)
        setStatus("error")
        setError("Lost connection to backend server.")
      }
    }, 1000)

    return () => clearInterval(pollRef.current)
  }, [scanId])

  const startScan = async () => {
    if (!path.trim()) return
    setError("")
    setLog([])
    setResults(null)
    setCurrentFile("")
    setParsedCount(0)
    setStatus("running")

    try {
      const res = await fetch(`${API_BASE}/api/scan`, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ path: path.trim() })
      })
      const data = await res.json()
      if (!res.ok) {
        setStatus("error")
        setError(data.error || "Failed to start scan.")
        return
      }
      setScanId(data.scan_id)
    } catch (e) {
      setStatus("error")
      setError("Cannot connect to backend. Make sure server.py is running.")
    }
  }

  const reset = () => {
    setPath("")
    setScanId(null)
    setStatus("idle")
    setResults(null)
    setLog([])
    setCurrentFile("")
    setParsedCount(0)
    setError("")
  }

  return (
    <div className="app">
      <header className="header">
        <div className="header-inner">
          <div className="logo-mark">FCG</div>
          <div>
            <h1 className="title">IP Finder</h1>
            <p className="subtitle">Automatic Patent Discovery</p>
          </div>
        </div>
      </header>

      <main className="main">
        {status === "idle" && (
          <div className="card scan-card">
            <h2 className="card-title">Scan a Directory</h2>
            <p className="card-desc">
              Enter the path to a local directory. IP Finder will scan all supported
              files and flag potential patentable inventions using AI analysis.
            </p>
            <div className="input-row">
              <input
                className="path-input"
                type="text"
                placeholder="e.g. C:\Users\Username\Projects\\my-app"
                value={path}
                onChange={e => setPath(e.target.value)}
                onKeyDown={e => e.key === "Enter" && startScan()}
              />
              <button className="btn-primary" onClick={startScan} disabled={!path.trim()}>
                Scan
              </button>
            </div>
            {error && <p className="error-msg">{error}</p>}
            <div className="supported-types">
              <span className="label">Supported types:</span>
              {[".py",".js",".ts",".java",".cs",".cpp",".c",".md",".txt",".json",".yaml",".yml"].map(ext => (
                <span key={ext} className="ext-tag">{ext}</span>
              ))}
            </div>
          </div>
        )}

        {status === "running" && (
          <div className="card scan-card">
            <div className="running-header">
              <div className="pulse-dot" />
              <h2 className="card-title">Scanning...</h2>
            </div>
            <p className="current-file">{currentFile || "Initializing scan..."}</p>
            <div className="stats-row">
              <div className="stat">
                <span className="stat-num">{parsedCount}</span>
                <span className="stat-label">Files Parsed</span>
              </div>
              <div className="stat">
                <span className="stat-num">{log.filter(l => l.includes("✓")).length}</span>
                <span className="stat-label">Candidates</span>
              </div>
            </div>
            <div className="log-box">
              {log.map((line, i) => (
                <div key={i} className={`log-line ${line.includes("✓") ? "log-hit" : ""}`}>
                  {line}
                </div>
              ))}
              <div ref={logEndRef} />
            </div>
          </div>
        )}

        {status === "complete" && results && (
          <div className="results">
            <div className="results-summary card">
              <h2 className="card-title">Scan Complete</h2>
              <div className="stats-row">
                <div className="stat">
                  <span className="stat-num">{results.parsed_files}</span>
                  <span className="stat-label">Files Parsed</span>
                </div>
                <div className="stat">
                  <span className="stat-num">{results.skipped_files}</span>
                  <span className="stat-label">Skipped (too large)</span>
                </div>
                <div className="stat highlight">
                  <span className="stat-num">{results.potential_ip_files.length}</span>
                  <span className="stat-label">IP Candidates</span>
                </div>
              </div>
              <button className="btn-secondary" onClick={reset}>New Scan</button>
            </div>

            {results.potential_ip_files.length === 0 ? (
              <div className="card empty-state">
                <p>No potential IP found in this directory.</p>
              </div>
            ) : (
              <div className="ip-list">
                <h3 className="section-title">Potential IP Files</h3>
                {results.potential_ip_files.map((item, i) => (
                  <div key={i} className="ip-card card">
                    <div className="ip-path">{item.file_path}</div>
                    <div className="keyword-row">
                      {item.keywords.map(kw => (
                        <span key={kw} className="kw-tag">{kw}</span>
                      ))}
                    </div>
                  </div>
                ))}
              </div>
            )}
          </div>
        )}

        {status === "error" && (
          <div className="card scan-card">
            <p className="error-msg">{error}</p>
            <button className="btn-secondary" onClick={reset}>Try Again</button>
          </div>
        )}
      </main>
    </div>
  )
}
