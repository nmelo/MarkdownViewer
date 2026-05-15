//
//  AppDelegate.swift
//  QLMarkdown
//
//  Standalone Markdown viewer with live file reload.
//

import Cocoa

class AppDelegate: NSObject, NSApplicationDelegate {
    private var mainWindowController: NSWindowController?
    private(set) var viewController: ViewController?

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

    func openFile(_ url: URL) {
        showMainWindow()
        _ = viewController?.openMarkdown(file: url)
    }

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
        var lastURL: URL?
        for i in 1...count {
            guard let item = list.atIndex(i) else { continue }
            // typeFileURL descriptors expose the URL via stringValue ("file:///…").
            if let s = item.stringValue, let url = URL(string: s), url.isFileURL {
                lastURL = url
            } else if let data = item.data as Data?,
                      let url = URL(dataRepresentation: data, relativeTo: nil) {
                lastURL = url
            }
        }
        if let url = lastURL {
            openFile(url)
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
        showMainWindow()
    }

    func application(_ sender: NSApplication, openFile filename: String) -> Bool {
        let file = URL(fileURLWithPath: filename)
        showMainWindow()
        return viewController?.openMarkdown(file: file) ?? false
    }

    func application(_ application: NSApplication, open urls: [URL]) {
        showMainWindow()
        // Single-window viewer: only the last URL wins.
        if let url = urls.last {
            _ = viewController?.openMarkdown(file: url)
        }
    }

    func applicationShouldTerminateAfterLastWindowClosed(_ sender: NSApplication) -> Bool {
        return true
    }

    func applicationShouldHandleReopen(_ sender: NSApplication, hasVisibleWindows flag: Bool) -> Bool {
        if !flag { showMainWindow() }
        return true
    }

    // MARK: - Window

    private func showMainWindow() {
        if mainWindowController == nil {
            let initialSize = NSSize(width: 900, height: 700)
            let vc = ViewController()
            // Force view to load before we attach it to a window.
            let v = vc.view
            v.frame = NSRect(origin: .zero, size: initialSize)
            let window = NSWindow(
                contentRect: NSRect(origin: .zero, size: initialSize),
                styleMask: [.titled, .closable, .miniaturizable, .resizable],
                backing: .buffered,
                defer: false
            )
            // Use contentView (not contentViewController) so AppKit doesn't try
            // to re-size the window from a view controller's preferredContentSize.
            window.contentView = v
            window.title = "Markdown Viewer"
            window.tabbingMode = .disallowed
            window.center()
            self.viewController = vc
            self.mainWindowController = NSWindowController(window: window)
        }
        mainWindowController?.window?.makeKeyAndOrderFront(nil)
        NSApp.activate(ignoringOtherApps: true)
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

        let openItem = NSMenuItem(title: "Open…",
                                  action: #selector(AppDelegate.openDocument(_:)),
                                  keyEquivalent: "o")
        openItem.target = self
        fileMenu.addItem(openItem)

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

    @objc func openDocument(_ sender: Any?) {
        showMainWindow()
        viewController?.openDocument(sender)
    }

    @objc func reloadDocument(_ sender: Any?) {
        viewController?.reloadDocument(sender)
    }
}

/// Intercepts AppKit's NSDocument-based open flow (which Finder "Open With"
/// triggers on apps that declare `CFBundleDocumentTypes`) and forwards the file
/// to our non-document viewer instead. Providing a stub `documentClass`
/// suppresses the "cannot open files in the X format" error that NSDocumentController
/// otherwise raises before it would ever call our open methods.
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
/// machinery: when it's asked to read a file, we hand the URL to the real viewer
/// and immediately tear the document down so no empty window is left around.
/// `@objc(MarkdownDocument)` makes the class discoverable via the `NSDocumentClass`
/// key in Info.plist's CFBundleDocumentTypes — required for NSDocumentController
/// to find it without an explicit Swift module prefix.
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
        // We render in the AppDelegate's main window, not via NSWindowController.
        // Close this document so AppKit doesn't track an invisible "open" doc.
        DispatchQueue.main.async { [weak self] in
            self?.close()
        }
    }
}
