@echo off
:: Check if Ollama is already installed
if exist "%LOCALAPPDATA%\Programs\Ollama\ollama.exe" (
    echo Ollama already installed, skipping.
    exit /b 0
)
:: Install Ollama silently
"%~dp0OllamaSetup.exe" /S
exit /b 0