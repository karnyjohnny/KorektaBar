import mainCpp from '../../native/src/main.cpp?raw';
import typesHpp from '../../native/src/Types.hpp?raw';
import configHpp from '../../native/src/Config.hpp?raw';
import configCpp from '../../native/src/Config.cpp?raw';
import textFormatterHpp from '../../native/src/TextFormatter.hpp?raw';
import textFormatterCpp from '../../native/src/TextFormatter.cpp?raw';
import polishAnalyzerHpp from '../../native/src/PolishAnalyzer.hpp?raw';
import polishAnalyzerCpp from '../../native/src/PolishAnalyzer.cpp?raw';
import spellCheckerWinHpp from '../../native/src/SpellCheckerWin.hpp?raw';
import spellCheckerWinCpp from '../../native/src/SpellCheckerWin.cpp?raw';
import backgroundWorkerHpp from '../../native/src/BackgroundWorker.hpp?raw';
import backgroundWorkerCpp from '../../native/src/BackgroundWorker.cpp?raw';
import d2dEditorHpp from '../../native/src/D2DEditor.hpp?raw';
import d2dEditorCpp from '../../native/src/D2DEditor.cpp?raw';
import settingsDialogHpp from '../../native/src/SettingsDialog.hpp?raw';
import settingsDialogCpp from '../../native/src/SettingsDialog.cpp?raw';
import mainWindowHpp from '../../native/src/MainWindow.hpp?raw';
import mainWindowCpp from '../../native/src/MainWindow.cpp?raw';
import makefileRaw from '../../native/Makefile?raw';
import cmakeRaw from '../../native/CMakeLists.txt?raw';
import resourceH from '../../native/resources/resource.h?raw';
import resourceRc from '../../native/resources/KorektaBar.rc?raw';
import appManifest from '../../native/resources/app.manifest?raw';
import releaseYml from '../../.github/workflows/release.yml?raw';
import readmeMd from '../../README.md?raw';
import generateAssetsPy from '../../scripts/generate_assets.py?raw';
import bannerSvg from '../../assets/banner.svg?raw';
import icoUrl from '../../assets/KorektaBar.ico?url';

export interface RepoFileEntry {
  path: string;
  group: 'C++ Rdzeń' | 'Zasoby i MSYS2' | 'GitHub i Skrypty';
  language: string;
  content: string;
}

export const REPO_FILES: RepoFileEntry[] = [
  { path: 'native/src/main.cpp', group: 'C++ Rdzeń', language: 'cpp', content: mainCpp },
  { path: 'native/src/MainWindow.hpp', group: 'C++ Rdzeń', language: 'cpp', content: mainWindowHpp },
  { path: 'native/src/MainWindow.cpp', group: 'C++ Rdzeń', language: 'cpp', content: mainWindowCpp },
  { path: 'native/src/D2DEditor.hpp', group: 'C++ Rdzeń', language: 'cpp', content: d2dEditorHpp },
  { path: 'native/src/D2DEditor.cpp', group: 'C++ Rdzeń', language: 'cpp', content: d2dEditorCpp },
  { path: 'native/src/BackgroundWorker.hpp', group: 'C++ Rdzeń', language: 'cpp', content: backgroundWorkerHpp },
  { path: 'native/src/BackgroundWorker.cpp', group: 'C++ Rdzeń', language: 'cpp', content: backgroundWorkerCpp },
  { path: 'native/src/PolishAnalyzer.hpp', group: 'C++ Rdzeń', language: 'cpp', content: polishAnalyzerHpp },
  { path: 'native/src/PolishAnalyzer.cpp', group: 'C++ Rdzeń', language: 'cpp', content: polishAnalyzerCpp },
  { path: 'native/src/SpellCheckerWin.hpp', group: 'C++ Rdzeń', language: 'cpp', content: spellCheckerWinHpp },
  { path: 'native/src/SpellCheckerWin.cpp', group: 'C++ Rdzeń', language: 'cpp', content: spellCheckerWinCpp },
  { path: 'native/src/TextFormatter.hpp', group: 'C++ Rdzeń', language: 'cpp', content: textFormatterHpp },
  { path: 'native/src/TextFormatter.cpp', group: 'C++ Rdzeń', language: 'cpp', content: textFormatterCpp },
  { path: 'native/src/SettingsDialog.hpp', group: 'C++ Rdzeń', language: 'cpp', content: settingsDialogHpp },
  { path: 'native/src/SettingsDialog.cpp', group: 'C++ Rdzeń', language: 'cpp', content: settingsDialogCpp },
  { path: 'native/src/Config.hpp', group: 'C++ Rdzeń', language: 'cpp', content: configHpp },
  { path: 'native/src/Config.cpp', group: 'C++ Rdzeń', language: 'cpp', content: configCpp },
  { path: 'native/src/Types.hpp', group: 'C++ Rdzeń', language: 'cpp', content: typesHpp },
  { path: 'native/Makefile', group: 'Zasoby i MSYS2', language: 'makefile', content: makefileRaw },
  { path: 'native/CMakeLists.txt', group: 'Zasoby i MSYS2', language: 'cmake', content: cmakeRaw },
  { path: 'native/resources/KorektaBar.rc', group: 'Zasoby i MSYS2', language: 'rc', content: resourceRc },
  { path: 'native/resources/resource.h', group: 'Zasoby i MSYS2', language: 'cpp', content: resourceH },
  { path: 'native/resources/app.manifest', group: 'Zasoby i MSYS2', language: 'xml', content: appManifest },
  { path: '.github/workflows/release.yml', group: 'GitHub i Skrypty', language: 'yaml', content: releaseYml },
  { path: 'README.md', group: 'GitHub i Skrypty', language: 'markdown', content: readmeMd },
  { path: 'scripts/generate_assets.py', group: 'GitHub i Skrypty', language: 'python', content: generateAssetsPy },
  { path: 'assets/banner.svg', group: 'GitHub i Skrypty', language: 'xml', content: bannerSvg },
];

export const ICO_ASSET_URL = icoUrl;
