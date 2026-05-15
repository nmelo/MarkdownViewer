//
//  ViewController.swift
//  MarkdownViewer
//
//  Standalone viewer: renders a markdown file and live-reloads on change.
//

import Cocoa
@preconcurrency import WebKit

class ViewController: NSViewController {
    private var webView: WKWebView!

    private(set) var markdownFile: URL? {
        didSet {
            updateWindowTitle()
            render()
            restartFileWatch()
        }
    }

    private var fileSource: DispatchSourceFileSystemObject?
    private var reloadDebounce: DispatchWorkItem?
    private var pendingScrollRestore: CGFloat = 0

    override func loadView() {
        let root = NSView(frame: NSRect(x: 0, y: 0, width: 900, height: 700))
        root.autoresizingMask = [.width, .height]

        let config = WKWebViewConfiguration()
        let settings = Settings.shared
        config.preferences.javaScriptEnabled =
            (settings.unsafeHTMLOption && settings.inlineImageExtension)
            || !settings.mermaidExtension.isDisabled
            || !settings.mathExtension.isDisabled
        config.allowsAirPlayForMediaPlayback = false

        let wv = WKWebView(frame: root.bounds, configuration: config)
        wv.autoresizingMask = [.width, .height]
        wv.navigationDelegate = self
        wv.allowsBackForwardNavigationGestures = false
        root.addSubview(wv)
        self.webView = wv

        self.view = root

        let drop = DropView(frame: root.bounds)
        drop.autoresizingMask = [.width, .height]
        drop.onDrop = { [weak self] url in self?.openMarkdown(file: url) }
        root.addSubview(drop)
    }

    override func viewDidAppear() {
        super.viewDidAppear()
        updateWindowTitle()
        if markdownFile == nil {
            showEmptyState()
        }
    }

    deinit {
        fileSource?.cancel()
    }

    @discardableResult
    func openMarkdown(file: URL) -> Bool {
        self.markdownFile = file
        NSDocumentController.shared.noteNewRecentDocumentURL(file)
        return true
    }

    @IBAction func openDocument(_ sender: Any?) {
        let panel = NSOpenPanel()
        panel.canChooseDirectories = false
        panel.canCreateDirectories = false
        panel.allowsMultipleSelection = false
        panel.allowedFileTypes = ["md", "markdown", "rmd", "qmd", "mdown", "mkd", "mkdn", "textbundle"]
        panel.message = "Select a Markdown file to view"
        if panel.runModal() == .OK, let url = panel.url {
            openMarkdown(file: url)
        }
    }

    @IBAction func reloadDocument(_ sender: Any?) {
        render()
    }

    private func updateWindowTitle() {
        view.window?.title = markdownFile?.lastPathComponent ?? AppDelegate.defaultWindowTitle
        view.window?.representedURL = markdownFile
    }

    private func render() {
        guard let file = markdownFile else {
            showEmptyState()
            return
        }

        webView.evaluateJavaScript("document.documentElement.scrollTop || document.body.scrollTop || 0") { [weak self] result, _ in
            guard let self = self else { return }
            let scroll = (result as? CGFloat) ?? 0
            self.pendingScrollRestore = scroll
            self.performRender(file: file)
        }
    }

    private func performRender(file: URL) {
        do {
            let settings = Settings.shared
            let resolved = Settings.getMarkdownFile(from: file)
            let appearance: Appearance = Settings.isLightAppearance ? .light : .dark
            let body = try settings.render(
                file: resolved,
                forAppearance: appearance,
                baseDir: resolved.deletingLastPathComponent().path
            )
            let html = settings.getCompleteHTML(title: file.lastPathComponent, body: body)
            webView.loadHTMLString(html, baseURL: resolved.deletingLastPathComponent())
        } catch {
            let safe = error.localizedDescription
                .replacingOccurrences(of: "&", with: "&amp;")
                .replacingOccurrences(of: "<", with: "&lt;")
            let html = """
            <html><body style="font-family: -apple-system, system-ui, sans-serif; padding: 2rem; color: #c0392b;">
              <h2>Render error</h2>
              <pre style="white-space: pre-wrap;">\(safe)</pre>
            </body></html>
            """
            webView.loadHTMLString(html, baseURL: nil)
        }
    }

    private func showEmptyState() {
        let html = """
        <html><body style="font-family: -apple-system, system-ui, sans-serif; display: flex; align-items: center; justify-content: center; height: 100vh; margin: 0; color: #888; background: #f6f6f6;">
          <div style="text-align: center;">
            <h2 style="font-weight: 300; margin-bottom: 0.5rem;">No file open</h2>
            <p>Open a Markdown file with <kbd>⌘O</kbd> or drag one onto the window.</p>
          </div>
        </body></html>
        """
        webView.loadHTMLString(html, baseURL: nil)
    }

    // MARK: - File watching

    private func restartFileWatch() {
        fileSource?.cancel()
        fileSource = nil

        guard let file = markdownFile else { return }

        // Watch the file itself.
        let fd = open(FileManager.default.fileSystemRepresentation(withPath: file.path), O_EVTONLY)
        if fd >= 0 {
            let src = DispatchSource.makeFileSystemObjectSource(
                fileDescriptor: fd,
                eventMask: [.write, .extend, .attrib, .delete, .rename, .revoke],
                queue: .main
            )
            src.setEventHandler { [weak self] in
                guard let self = self, let src = self.fileSource else { return }
                let flags = src.data
                // Inode replaced (delete/rename): re-open the watch so future saves still fire.
                let reopen = !flags.intersection([.delete, .rename, .revoke]).isEmpty
                self.scheduleReload(reopenFile: reopen)
            }
            src.setCancelHandler { close(fd) }
            src.resume()
            self.fileSource = src
        }

        // We intentionally do NOT also open the parent directory: that triggers
        // a TCC "App Management" / cross-container consent prompt every time the
        // file lives somewhere protected (iCloud, ~/Documents, another app's
        // container, …). Atomic-rename saves are still handled — the file
        // source above fires .delete/.rename when the inode is replaced and
        // scheduleReload(reopenFile: true) re-opens the new inode.
    }

    private func scheduleReload(reopenFile: Bool) {
        reloadDebounce?.cancel()
        let work = DispatchWorkItem { [weak self] in
            guard let self = self, let file = self.markdownFile else { return }
            // Skip if the file is gone (mid-rename); the next event after the rename completes will fire again.
            guard FileManager.default.fileExists(atPath: file.path) else { return }
            if reopenFile {
                self.restartFileWatch()
            }
            self.render()
        }
        reloadDebounce = work
        DispatchQueue.main.asyncAfter(deadline: .now() + .milliseconds(120), execute: work)
    }
}

extension ViewController: WKNavigationDelegate {
    func webView(_ webView: WKWebView, didFinish navigation: WKNavigation!) {
        if pendingScrollRestore > 0 {
            let y = pendingScrollRestore
            pendingScrollRestore = 0
            webView.evaluateJavaScript("window.scrollTo(0, \(y));")
        }
    }

    func webView(_ webView: WKWebView, decidePolicyFor navigationAction: WKNavigationAction, decisionHandler: @escaping (WKNavigationActionPolicy) -> Void) {
        if navigationAction.navigationType == .linkActivated,
           let url = navigationAction.request.url,
           url.scheme != "file" {
            NSWorkspace.shared.open(url)
            decisionHandler(.cancel)
            return
        }
        decisionHandler(.allow)
    }
}

// MARK: - Drop target overlay

final class DropView: NSView {
    var onDrop: ((URL) -> Void)?

    override init(frame frameRect: NSRect) {
        super.init(frame: frameRect)
        registerForDraggedTypes([.fileURL])
    }

    required init?(coder: NSCoder) {
        super.init(coder: coder)
        registerForDraggedTypes([.fileURL])
    }

    override func hitTest(_ point: NSPoint) -> NSView? {
        // Transparent to clicks; only intercepts drags.
        return nil
    }

    override func draggingEntered(_ sender: NSDraggingInfo) -> NSDragOperation {
        return urlFromDrag(sender) != nil ? .copy : []
    }

    override func performDragOperation(_ sender: NSDraggingInfo) -> Bool {
        guard let url = urlFromDrag(sender) else { return false }
        onDrop?(url)
        return true
    }

    private func urlFromDrag(_ sender: NSDraggingInfo) -> URL? {
        guard let urls = sender.draggingPasteboard.readObjects(forClasses: [NSURL.self]) as? [URL],
              let url = urls.first else { return nil }
        let ext = url.pathExtension.lowercased()
        let known: Set<String> = ["md", "markdown", "rmd", "qmd", "mdown", "mkd", "mkdn", "textbundle"]
        return known.contains(ext) ? url : nil
    }
}
