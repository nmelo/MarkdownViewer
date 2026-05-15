//
//  AppDelegate.swift
//  MarkdownViewer
//
//  Standalone Markdown viewer with live file reload + multi-window.
//

import Cocoa

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
        let v = vc.view
        v.frame = NSRect(origin: .zero, size: initialSize)
        let window = NSWindow(
            contentRect: NSRect(origin: .zero, size: initialSize),
            styleMask: [.titled, .closable, .miniaturizable, .resizable],
            backing: .buffered,
            defer: false
        )
        window.contentView = v
        window.title = AppDelegate.defaultWindowTitle
        window.tabbingMode = .preferred
        window.isReleasedWhenClosed = false

        // Cascade so consecutive new windows don't stack on top of each other.
        if let previous = owners.last?.windowController.window {
            let topLeft = previous.cascadeTopLeft(from: .zero)
            window.cascadeTopLeft(from: topLeft)
        } else {
            window.center()
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

        NSApp.mainMenu = mainMenu
    }

    @objc func newDocument(_ sender: Any?) {
        _ = openNewWindow()
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
