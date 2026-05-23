//
//  AppDelegate.swift
//  MarkdownViewer
//
//  Standalone Markdown viewer with live file reload + multi-window.
//

import Cocoa
@preconcurrency import WebKit
import Sparkle

class AppDelegate: NSObject, NSApplicationDelegate {
    /// Open windows, each backing one document (or an empty draft).
    private struct Owner {
        let windowController: NSWindowController
        let viewController: ViewController
    }

    private var owners: [Owner] = []

    // Installing this subclass before AppKit asks for the shared instance
    // prevents the default NSDocumentController from rejecting our file types
    // with "cannot open files in the 'Markdown Document' format".
    private var documentController: ViewerDocumentController?

    /// Sparkle auto-updater. Polls the feed URL declared in Info.plist
    /// (`SUFeedURL`), verifies signatures against `SUPublicEDKey`, prompts
    /// the user, and installs in-place when the user confirms.
    private(set) lazy var updaterController: SPUStandardUpdaterController =
        SPUStandardUpdaterController(startingUpdater: true, updaterDelegate: nil, userDriverDelegate: nil)

    override init() {
        super.init()
        self.documentController = ViewerDocumentController()
    }

    func applicationSupportsSecureRestorableState(_ app: NSApplication) -> Bool {
        return false
    }

    func applicationShouldOpenUntitledFile(_ sender: NSApplication) -> Bool {
        return false
    }

    static let mainFrameDefaultsKey = "MainWindowFrame"

    static var defaultWindowTitle: String {
        let info = Bundle.main.infoDictionary
        let version = info?["CFBundleShortVersionString"] as? String ?? ""
        return version.isEmpty ? "Markdown Viewer" : "Markdown Viewer \(version)"
    }

    // MARK: - Lifecycle

    func applicationWillFinishLaunching(_ notification: Notification) {
        // Install our Apple Event handler BEFORE NSDocumentController installs its
        // default one. Otherwise Finder "Open With" → 'odoc' events hit
        // NSDocumentController's handler, which rejects unknown types with
        // "cannot open files in the … format" because we ship no NSDocument subclass.
        NSAppleEventManager.shared().setEventHandler(
            self,
            andSelector: #selector(handleOpenDocumentsEvent(_:withReplyEvent:)),
            forEventClass: AEEventClass(kCoreEventClass),
            andEventID: AEEventID(kAEOpenDocuments)
        )
    }

    @objc func handleOpenDocumentsEvent(_ event: NSAppleEventDescriptor, withReplyEvent reply: NSAppleEventDescriptor) {
        guard let list = event.paramDescriptor(forKeyword: keyDirectObject) else { return }
        let count = list.numberOfItems
        guard count > 0 else { return }
        for i in 1...count {
            guard let item = list.atIndex(i) else { continue }
            // typeFileURL descriptors expose the URL via stringValue ("file:///…").
            if let s = item.stringValue, let url = URL(string: s), url.isFileURL {
                openFile(url)
            } else if let data = item.data as Data?,
                      let url = URL(dataRepresentation: data, relativeTo: nil) {
                openFile(url)
            }
        }
    }

    func applicationDidFinishLaunching(_ aNotification: Notification) {
        // Touching the lazy var kicks the updater off so it starts polling on
        // its scheduled interval (SUScheduledCheckInterval in Info.plist).
        _ = updaterController
        Settings.shared.installDependencies()

        if let path = Settings.mermaidCacheFileUrl, !FileManager.default.fileExists(atPath: path.path) {
            Settings.shared.updateMemaidCache { _ in }
        }
        if let path = Settings.mathJaxCacheFileUrl, !FileManager.default.fileExists(atPath: path.path) {
            Settings.shared.updateMathJaxUCache { _ in }
        }

        buildMenu()

        // If launch didn't bring any windows in (no file dragged on the icon, no
        // "open" Apple Event), start with one empty window.
        if owners.isEmpty {
            _ = openNewWindow()
        }
    }

    func application(_ sender: NSApplication, openFile filename: String) -> Bool {
        openFile(URL(fileURLWithPath: filename))
        return true
    }

    func application(_ application: NSApplication, open urls: [URL]) {
        for url in urls {
            openFile(url)
        }
    }

    func applicationShouldTerminateAfterLastWindowClosed(_ sender: NSApplication) -> Bool {
        return true
    }

    func applicationShouldHandleReopen(_ sender: NSApplication, hasVisibleWindows flag: Bool) -> Bool {
        if !flag { _ = openNewWindow() }
        return true
    }

    // MARK: - Window orchestration

    /// Open the given file. Reuses an existing window if one already shows that
    /// path; otherwise reuses the front-most empty window if there is one;
    /// otherwise creates a new window.
    func openFile(_ url: URL) {
        let std = url.standardizedFileURL

        if let existing = owners.first(where: {
            $0.viewController.markdownFile?.standardizedFileURL == std
        }) {
            existing.windowController.window?.makeKeyAndOrderFront(nil)
            NSApp.activate(ignoringOtherApps: true)
            return
        }

        if let empty = owners.first(where: { $0.viewController.markdownFile == nil }) {
            _ = empty.viewController.openMarkdown(file: url)
            empty.windowController.window?.makeKeyAndOrderFront(nil)
            NSApp.activate(ignoringOtherApps: true)
            return
        }

        let owner = openNewWindow()
        _ = owner.viewController.openMarkdown(file: url)
        NSApp.activate(ignoringOtherApps: true)
    }

    @discardableResult
    private func openNewWindow() -> Owner {
        let initialSize = NSSize(width: 900, height: 700)
        let vc = ViewController()
        let window = NSWindow(
            contentRect: NSRect(origin: .zero, size: initialSize),
            styleMask: [.titled, .closable, .miniaturizable, .resizable],
            backing: .buffered,
            defer: false
        )
        // contentViewController (not contentView) so the VC sits in the window's
        // responder chain — required for menu actions like toggleTOC(_:) that
        // live on the split-view controller, above the content child VC.
        window.contentViewController = vc
        // Assigning the content can shrink the window to AL-intrinsic size of
        // any subview that uses Auto Layout (the FindBar does). Force the
        // intended content size back.
        window.setContentSize(initialSize)
        window.contentMinSize = NSSize(width: 480, height: 320)
        window.title = AppDelegate.defaultWindowTitle
        window.tabbingMode = .disallowed
        window.isReleasedWhenClosed = false

        // Remember position + size across launches. Only the first window of
        // each session restores from defaults; subsequent windows cascade from
        // the previous window in this session.
        if owners.isEmpty {
            if let str = UserDefaults.standard.string(forKey: AppDelegate.mainFrameDefaultsKey) {
                let saved = NSRectFromString(str)
                // Make sure the saved frame still falls on an available screen.
                if NSScreen.screens.contains(where: { $0.visibleFrame.intersects(saved) }) {
                    window.setFrame(saved, display: false)
                } else {
                    window.center()
                }
            } else {
                window.center()
            }
        } else if let previous = owners.last?.windowController.window {
            let topLeft = previous.cascadeTopLeft(from: .zero)
            window.cascadeTopLeft(from: topLeft)
        } else {
            window.center()
        }

        // Persist this window's frame whenever it moves or resizes. We only
        // track the first window of the session so the "remembered" frame
        // matches the user's primary one rather than the cascaded extras.
        let isFirstWindow = owners.isEmpty
        if isFirstWindow {
            let persist = { [weak window] in
                guard let w = window else { return }
                UserDefaults.standard.set(NSStringFromRect(w.frame), forKey: AppDelegate.mainFrameDefaultsKey)
            }
            for name in [NSWindow.didResizeNotification, NSWindow.didMoveNotification] {
                NotificationCenter.default.addObserver(forName: name, object: window, queue: .main) { _ in persist() }
            }
        }

        let wc = NSWindowController(window: window)
        let owner = Owner(windowController: wc, viewController: vc)
        owners.append(owner)

        NotificationCenter.default.addObserver(
            forName: NSWindow.willCloseNotification,
            object: window,
            queue: .main
        ) { [weak self] _ in
            self?.owners.removeAll { $0.windowController === wc }
        }

        wc.window?.makeKeyAndOrderFront(nil)
        return owner
    }

    /// The window controller / view controller pair that owns the key window,
    /// falling back to the most recently created one if nothing is key.
    private var frontOwner: Owner? {
        if let keyWindow = NSApp.keyWindow,
           let match = owners.first(where: { $0.windowController.window === keyWindow }) {
            return match
        }
        return owners.last
    }

    // MARK: - Menu

    private func buildMenu() {
        let appName = ProcessInfo.processInfo.processName
        let mainMenu = NSMenu()

        // App menu
        let appItem = NSMenuItem()
        mainMenu.addItem(appItem)
        let appMenu = NSMenu(title: appName)
        appItem.submenu = appMenu
        appMenu.addItem(withTitle: "About \(appName)",
                        action: #selector(NSApplication.orderFrontStandardAboutPanel(_:)),
                        keyEquivalent: "")
        appMenu.addItem(.separator())
        // Sparkle: "Check for Updates…" wired to the standard updater
        // controller. The selector lives on SPUStandardUpdaterController.
        let checkForUpdates = NSMenuItem(title: "Check for Updates…",
                                         action: #selector(SPUStandardUpdaterController.checkForUpdates(_:)),
                                         keyEquivalent: "")
        checkForUpdates.target = updaterController
        appMenu.addItem(checkForUpdates)
        appMenu.addItem(.separator())
        let services = NSMenuItem(title: "Services", action: nil, keyEquivalent: "")
        let servicesMenu = NSMenu(title: "Services")
        services.submenu = servicesMenu
        appMenu.addItem(services)
        NSApp.servicesMenu = servicesMenu
        appMenu.addItem(.separator())
        appMenu.addItem(withTitle: "Hide \(appName)",
                        action: #selector(NSApplication.hide(_:)),
                        keyEquivalent: "h")
        let hideOthers = NSMenuItem(title: "Hide Others",
                                    action: #selector(NSApplication.hideOtherApplications(_:)),
                                    keyEquivalent: "h")
        hideOthers.keyEquivalentModifierMask = [.command, .option]
        appMenu.addItem(hideOthers)
        appMenu.addItem(withTitle: "Show All",
                        action: #selector(NSApplication.unhideAllApplications(_:)),
                        keyEquivalent: "")
        appMenu.addItem(.separator())
        appMenu.addItem(withTitle: "Quit \(appName)",
                        action: #selector(NSApplication.terminate(_:)),
                        keyEquivalent: "q")

        // File menu
        let fileItem = NSMenuItem()
        mainMenu.addItem(fileItem)
        let fileMenu = NSMenu(title: "File")
        fileItem.submenu = fileMenu

        let newItem = NSMenuItem(title: "New Window",
                                 action: #selector(AppDelegate.newDocument(_:)),
                                 keyEquivalent: "n")
        newItem.target = self
        fileMenu.addItem(newItem)

        let openItem = NSMenuItem(title: "Open…",
                                  action: #selector(AppDelegate.openDocument(_:)),
                                  keyEquivalent: "o")
        openItem.target = self
        fileMenu.addItem(openItem)

        // AppKit auto-populates "Open Recent" using the NSDocumentController's
        // recent-documents list, as long as the submenu is titled exactly that.
        let openRecent = NSMenuItem(title: "Open Recent", action: nil, keyEquivalent: "")
        let openRecentMenu = NSMenu(title: "Open Recent")
        openRecent.submenu = openRecentMenu
        let clearRecent = NSMenuItem(title: "Clear Menu",
                                     action: #selector(NSDocumentController.clearRecentDocuments(_:)),
                                     keyEquivalent: "")
        openRecentMenu.addItem(clearRecent)
        fileMenu.addItem(openRecent)

        fileMenu.addItem(.separator())
        fileMenu.addItem(withTitle: "Close",
                         action: #selector(NSWindow.performClose(_:)),
                         keyEquivalent: "w")
        let reloadItem = NSMenuItem(title: "Reload",
                                    action: #selector(AppDelegate.reloadDocument(_:)),
                                    keyEquivalent: "r")
        reloadItem.target = self
        fileMenu.addItem(reloadItem)

        // Edit menu (so Cmd-C, Cmd-A work in the rendered HTML)
        let editItem = NSMenuItem()
        mainMenu.addItem(editItem)
        let editMenu = NSMenu(title: "Edit")
        editItem.submenu = editMenu
        editMenu.addItem(withTitle: "Copy", action: #selector(NSText.copy(_:)), keyEquivalent: "c")
        editMenu.addItem(withTitle: "Select All", action: #selector(NSText.selectAll(_:)), keyEquivalent: "a")
        editMenu.addItem(.separator())
        // Find submenu — selectors fire on the firstResponder, which is the front window's ViewController.
        let findSub = NSMenu(title: "Find")
        let findItem = NSMenuItem(title: "Find", action: nil, keyEquivalent: "")
        findItem.submenu = findSub
        editMenu.addItem(findItem)
        findSub.addItem(withTitle: "Find…",
                        action: #selector(ViewController.performFind(_:)),
                        keyEquivalent: "f")
        let findNext = NSMenuItem(title: "Find Next",
                                  action: #selector(ViewController.findNext(_:)),
                                  keyEquivalent: "g")
        findSub.addItem(findNext)
        let findPrev = NSMenuItem(title: "Find Previous",
                                  action: #selector(ViewController.findPrevious(_:)),
                                  keyEquivalent: "g")
        findPrev.keyEquivalentModifierMask = [.command, .shift]
        findSub.addItem(findPrev)

        // View menu
        let viewItem = NSMenuItem()
        mainMenu.addItem(viewItem)
        let viewMenu = NSMenu(title: "View")
        viewItem.submenu = viewMenu
        viewMenu.addItem(withTitle: "Zoom In",
                         action: #selector(ViewController.zoomIn(_:)),
                         keyEquivalent: "+")
        viewMenu.addItem(withTitle: "Zoom Out",
                         action: #selector(ViewController.zoomOut(_:)),
                         keyEquivalent: "-")
        viewMenu.addItem(withTitle: "Actual Size",
                         action: #selector(ViewController.zoomReset(_:)),
                         keyEquivalent: "0")
        viewMenu.addItem(.separator())
        let wideItem = NSMenuItem(title: "Toggle Wide",
                                  action: #selector(ViewController.toggleWide(_:)),
                                  keyEquivalent: "w")
        wideItem.keyEquivalentModifierMask = [.command, .shift]
        viewMenu.addItem(wideItem)

        viewMenu.addItem(.separator())
        // ⌃⌘S matches AppKit's standard "Show/Hide Sidebar" shortcut.
        let tocToggle = NSMenuItem(title: "Show Table of Contents",
                                   action: #selector(ViewController.toggleTOC(_:)),
                                   keyEquivalent: "s")
        tocToggle.keyEquivalentModifierMask = [.command, .control]
        viewMenu.addItem(tocToggle)

        let tocPos = NSMenuItem(title: "Move Sidebar to Right",
                                action: #selector(ViewController.toggleTOCPosition(_:)),
                                keyEquivalent: "")
        viewMenu.addItem(tocPos)

        // Window menu
        let windowItem = NSMenuItem()
        mainMenu.addItem(windowItem)
        let windowMenu = NSMenu(title: "Window")
        windowItem.submenu = windowMenu
        windowMenu.addItem(withTitle: "Minimize",
                           action: #selector(NSWindow.performMiniaturize(_:)),
                           keyEquivalent: "m")
        windowMenu.addItem(withTitle: "Zoom",
                           action: #selector(NSWindow.performZoom(_:)),
                           keyEquivalent: "")
        windowMenu.addItem(.separator())
        windowMenu.addItem(withTitle: "Bring All to Front",
                           action: #selector(NSApplication.arrangeInFront(_:)),
                           keyEquivalent: "")
        NSApp.windowsMenu = windowMenu

        // Help menu
        let helpItem = NSMenuItem()
        mainMenu.addItem(helpItem)
        let helpMenu = NSMenu(title: "Help")
        helpItem.submenu = helpMenu
        let helpEntry = NSMenuItem(title: "Markdown Viewer Help",
                                   action: #selector(AppDelegate.showHelp(_:)),
                                   keyEquivalent: "?")
        helpEntry.keyEquivalentModifierMask = [.command]
        helpEntry.target = self
        helpMenu.addItem(helpEntry)
        NSApp.helpMenu = helpMenu

        NSApp.mainMenu = mainMenu
    }

    @objc func newDocument(_ sender: Any?) {
        _ = openNewWindow()
        NSApp.activate(ignoringOtherApps: true)
    }

    // MARK: - Help

    private var helpPanel: NSPanel?

    @objc func showHelp(_ sender: Any?) {
        if helpPanel == nil {
            helpPanel = HelpPanel.make()
        }
        helpPanel?.center()
        helpPanel?.makeKeyAndOrderFront(nil)
        NSApp.activate(ignoringOtherApps: true)
    }

    @objc func openDocument(_ sender: Any?) {
        let panel = NSOpenPanel()
        panel.canChooseDirectories = false
        panel.canCreateDirectories = false
        panel.allowsMultipleSelection = true
        panel.allowedFileTypes = ["md", "markdown", "rmd", "qmd", "mdown", "mkd", "mkdn", "textbundle"]
        panel.message = "Select Markdown files to view"
        if panel.runModal() == .OK {
            for url in panel.urls {
                openFile(url)
            }
        }
    }

    @objc func reloadDocument(_ sender: Any?) {
        frontOwner?.viewController.reloadDocument(sender)
    }
}

/// Intercepts AppKit's NSDocument-based open flow (which Finder "Open With"
/// triggers on apps that declare `CFBundleDocumentTypes`) and forwards each file
/// to AppDelegate.openFile so the multi-window logic decides where it lands.
final class ViewerDocumentController: NSDocumentController {
    override func documentClass(forType typeName: String) -> AnyClass? {
        return MarkdownDocument.self
    }

    override func openDocument(withContentsOf url: URL,
                               display displayDocument: Bool,
                               completionHandler: @escaping (NSDocument?, Bool, Error?) -> Void) {
        DispatchQueue.main.async {
            (NSApp.delegate as? AppDelegate)?.openFile(url)
            completionHandler(nil, true, nil)
        }
    }

    override func reopenDocument(for urlOrNil: URL?,
                                 withContentsOf contentsURL: URL,
                                 display displayDocument: Bool,
                                 completionHandler: @escaping (NSDocument?, Bool, Error?) -> Void) {
        DispatchQueue.main.async {
            (NSApp.delegate as? AppDelegate)?.openFile(contentsURL)
            completionHandler(nil, true, nil)
        }
    }
}

/// A small floating panel that shows keyboard shortcuts + app info, opened
/// with ⌘? from the Help menu. Lives apart from any document window so it's
/// available even when no file is open.
enum HelpPanel {
    static func make() -> NSPanel {
        let size = NSSize(width: 460, height: 540)
        let panel = NSPanel(
            contentRect: NSRect(origin: .zero, size: size),
            styleMask: [.titled, .closable, .resizable, .fullSizeContentView, .hudWindow],
            backing: .buffered,
            defer: false
        )
        panel.title = "Markdown Viewer Help"
        panel.titleVisibility = .hidden
        panel.titlebarAppearsTransparent = true
        panel.isFloatingPanel = true
        panel.becomesKeyOnlyIfNeeded = false
        panel.hidesOnDeactivate = false
        panel.isReleasedWhenClosed = false

        let webView = WKWebView(frame: NSRect(origin: .zero, size: size))
        webView.autoresizingMask = [.width, .height]
        webView.setValue(false, forKey: "drawsBackground")
        webView.loadHTMLString(html(), baseURL: nil)
        panel.contentView = webView

        return panel
    }

    private static func html() -> String {
        let info = Bundle.main.infoDictionary
        let version = info?["CFBundleShortVersionString"] as? String ?? ""
        let build = info?["CFBundleVersion"] as? String ?? ""
        let copyright = info?["NSHumanReadableCopyright"] as? String ?? ""

        return """
        <!doctype html>
        <html>
        <head>
          <meta charset="utf-8">
          <style>
            :root { color-scheme: light dark; }
            html, body { margin: 0; height: 100%; }
            body {
              font-family: -apple-system, system-ui, sans-serif;
              padding: 22px 26px;
              line-height: 1.45;
              color: #1d1d1f; background: rgba(245,245,247,0.94);
            }
            h1 { font-size: 17px; margin: 0 0 4px; font-weight: 600; }
            .version { font-size: 12px; color: #6b6b6f; margin-bottom: 18px; }
            h2 { font-size: 12px; text-transform: uppercase; letter-spacing: 0.06em;
                 color: #6b6b6f; margin: 18px 0 8px; font-weight: 600; }
            table { width: 100%; border-collapse: collapse; font-size: 13px; }
            td { padding: 4px 0; vertical-align: top; }
            td.key {
              white-space: nowrap; padding-right: 14px; color: #1d1d1f;
              font-family: ui-monospace, Menlo, monospace; font-size: 12px;
            }
            kbd {
              display: inline-block; padding: 1px 6px; min-width: 18px; text-align: center;
              border-radius: 4px; background: rgba(0,0,0,0.08);
              font-family: ui-monospace, Menlo, monospace; font-size: 11px;
            }
            a { color: #0366d6; text-decoration: none; }
            a:hover { text-decoration: underline; }
            .foot { margin-top: 18px; font-size: 11px; color: #6b6b6f; }
            @media (prefers-color-scheme: dark) {
              body { color: #f0f6fc; background: rgba(20,20,22,0.92); }
              h2, .foot, .version { color: #8b949e; }
              td.key { color: #f0f6fc; }
              kbd { background: rgba(255,255,255,0.10); color: #f0f6fc; }
              a { color: #58a6ff; }
            }
          </style>
        </head>
        <body>
          <h1>Markdown Viewer</h1>
          <div class="version">Version \(version) (\(build))</div>

          <h2>File</h2>
          <table>
            <tr><td class="key"><kbd>⌘</kbd> <kbd>N</kbd></td><td>New window</td></tr>
            <tr><td class="key"><kbd>⌘</kbd> <kbd>O</kbd></td><td>Open file…</td></tr>
            <tr><td class="key"><kbd>⌘</kbd> <kbd>R</kbd></td><td>Reload current document</td></tr>
            <tr><td class="key"><kbd>⌘</kbd> <kbd>W</kbd></td><td>Close window</td></tr>
          </table>

          <h2>Find</h2>
          <table>
            <tr><td class="key"><kbd>⌘</kbd> <kbd>F</kbd></td><td>Find in page</td></tr>
            <tr><td class="key"><kbd>⌘</kbd> <kbd>G</kbd></td><td>Find next</td></tr>
            <tr><td class="key"><kbd>⌘</kbd> <kbd>⇧</kbd> <kbd>G</kbd></td><td>Find previous</td></tr>
            <tr><td class="key"><kbd>esc</kbd></td><td>Dismiss find bar</td></tr>
          </table>

          <h2>View</h2>
          <table>
            <tr><td class="key"><kbd>⌘</kbd> <kbd>+</kbd></td><td>Zoom in</td></tr>
            <tr><td class="key"><kbd>⌘</kbd> <kbd>−</kbd></td><td>Zoom out</td></tr>
            <tr><td class="key"><kbd>⌘</kbd> <kbd>0</kbd></td><td>Actual size</td></tr>
            <tr><td class="key">pinch</td><td>Zoom (trackpad)</td></tr>
            <tr><td class="key"><kbd>⌘</kbd> <kbd>⇧</kbd> <kbd>W</kbd></td><td>Toggle wide / max-width</td></tr>
            <tr><td class="key"><kbd>⌃</kbd> <kbd>⌘</kbd> <kbd>S</kbd></td><td>Show/hide table of contents</td></tr>
          </table>

          <h2>Help</h2>
          <table>
            <tr><td class="key"><kbd>⌘</kbd> <kbd>?</kbd></td><td>Show this panel</td></tr>
          </table>

          <div class="foot">
            <a href="https://github.com/nmelo/MarkdownViewer">github.com/nmelo/MarkdownViewer</a><br>
            \(copyright)
          </div>
        </body>
        </html>
        """
    }
}

/// A no-op `NSDocument` whose only purpose is to satisfy AppKit's document
/// machinery: when it's asked to read a file, we hand the URL to the real
/// viewer and immediately tear the document down so no empty window is left.
@objc(MarkdownDocument)
final class MarkdownDocument: NSDocument {
    override class var autosavesInPlace: Bool { false }
    override class var preservesVersions: Bool { false }

    override func read(from url: URL, ofType typeName: String) throws {
        let captured = url
        DispatchQueue.main.async {
            (NSApp.delegate as? AppDelegate)?.openFile(captured)
        }
    }

    override func makeWindowControllers() {
        DispatchQueue.main.async { [weak self] in
            self?.close()
        }
    }
}
