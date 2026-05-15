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
    private var findBar: FindBar!

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
        // Enable right-click → Inspect Element so we can debug the rendered HTML.
        config.preferences.setValue(true, forKey: "developerExtrasEnabled")

        let wv = WKWebView(frame: root.bounds, configuration: config)
        wv.autoresizingMask = [.width, .height]
        wv.navigationDelegate = self
        wv.allowsBackForwardNavigationGestures = false
        wv.allowsMagnification = true   // trackpad pinch-to-zoom
        root.addSubview(wv)
        self.webView = wv

        // Drop overlay accepts drag-and-drop without intercepting clicks.
        let drop = DropView(frame: root.bounds)
        drop.autoresizingMask = [.width, .height]
        drop.onDrop = { [weak self] url in self?.openMarkdown(file: url) }
        root.addSubview(drop)

        // Find bar overlays the top of the window. Hidden until ⌘F.
        let bar = FindBar()
        bar.translatesAutoresizingMaskIntoConstraints = true
        bar.frame = NSRect(x: 0, y: root.bounds.height - FindBar.barHeight,
                           width: root.bounds.width, height: FindBar.barHeight)
        bar.autoresizingMask = [.width, .minYMargin]
        bar.isHidden = true
        bar.delegate = self
        root.addSubview(bar)
        self.findBar = bar

        self.view = root
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

    @IBAction func reloadDocument(_ sender: Any?) {
        render()
    }

    // MARK: - Find

    @objc func performFind(_ sender: Any?) {
        findBar.isHidden = false
        view.window?.makeFirstResponder(findBar.searchField)
    }

    @objc func findNext(_ sender: Any?) {
        runFind(direction: .forward)
    }

    @objc func findPrevious(_ sender: Any?) {
        runFind(direction: .backward)
    }

    private enum FindDirection { case forward, backward }

    private func runFind(direction: FindDirection) {
        let query = findBar.searchField.stringValue
        guard !query.isEmpty else {
            findBar.setStatus(.idle)
            return
        }
        let config = WKFindConfiguration()
        config.backwards = direction == .backward
        config.caseSensitive = false
        config.wraps = true
        webView.find(query, configuration: config) { [weak self] result in
            self?.findBar.setStatus(result.matchFound ? .found : .notFound)
        }
    }

    fileprivate func dismissFind() {
        findBar.isHidden = true
        findBar.searchField.stringValue = ""
        findBar.setStatus(.idle)
        view.window?.makeFirstResponder(webView)
    }

    // MARK: - Zoom

    @objc func zoomIn(_ sender: Any?) {
        webView.pageZoom = min(webView.pageZoom + 0.1, 3.0)
    }

    @objc func zoomOut(_ sender: Any?) {
        webView.pageZoom = max(webView.pageZoom - 0.1, 0.4)
    }

    @objc func zoomReset(_ sender: Any?) {
        webView.pageZoom = 1.0
    }

    // MARK: - Wide toggle

    @objc func toggleWide(_ sender: Any?) {
        // Flip an "is-wide" class on the rendered <article>. The CSS rule
        // `article.wide { max-width: none; }` (see default.css) removes the
        // 902-px cap so the document fills the window.
        webView.evaluateJavaScript("""
        (function(){
            const a = document.querySelector('article');
            if (a) a.classList.toggle('wide');
        })();
        """)
    }

    // MARK: - Rendering

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
        <html>
        <head>
          <meta name="color-scheme" content="light dark">
          <style>
            :root { color-scheme: light dark; }
            html, body { height: 100%; margin: 0; }
            body {
              font-family: -apple-system, system-ui, sans-serif;
              display: flex; align-items: center; justify-content: center;
              background: #f6f6f6; color: #888;
            }
            kbd {
              padding: 1px 6px; border-radius: 4px;
              background: rgba(0,0,0,0.06);
              font-family: ui-monospace, Menlo, monospace; font-size: 0.95em;
            }
            @media (prefers-color-scheme: dark) {
              body { background: #0d1117; color: #8b949e; }
              kbd { background: rgba(255,255,255,0.08); }
            }
          </style>
        </head>
        <body>
          <div style="text-align: center;">
            <h2 style="font-weight: 300; margin-bottom: 0.5rem;">No file open</h2>
            <p>Open a Markdown file with <kbd>⌘O</kbd> or drag one onto the window.</p>
          </div>
        </body>
        </html>
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

extension ViewController: FindBarDelegate {
    func findBarSearchTextChanged(_ text: String) {
        // Live-find while typing.
        if text.isEmpty {
            findBar.setStatus(.idle)
            return
        }
        runFind(direction: .forward)
    }

    func findBarNextRequested() { runFind(direction: .forward) }
    func findBarPreviousRequested() { runFind(direction: .backward) }
    func findBarCloseRequested() { dismissFind() }
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

// MARK: - Find bar

protocol FindBarDelegate: AnyObject {
    func findBarSearchTextChanged(_ text: String)
    func findBarNextRequested()
    func findBarPreviousRequested()
    func findBarCloseRequested()
}

final class FindBar: NSView, NSSearchFieldDelegate {
    static let barHeight: CGFloat = 38

    weak var delegate: FindBarDelegate?

    let searchField = NSSearchField()
    private let statusLabel = NSTextField(labelWithString: "")
    private let prevButton = NSButton()
    private let nextButton = NSButton()
    private let closeButton = NSButton()
    private let divider = NSBox()

    enum Status { case idle, found, notFound }

    func setStatus(_ status: Status) {
        switch status {
        case .idle, .found: statusLabel.stringValue = ""
        case .notFound:     statusLabel.stringValue = "Not found"
        }
    }

    override init(frame frameRect: NSRect) {
        super.init(frame: frameRect)
        setup()
    }

    required init?(coder: NSCoder) {
        super.init(coder: coder)
        setup()
    }

    private func setup() {
        wantsLayer = true
        layer?.backgroundColor = NSColor.windowBackgroundColor.cgColor

        searchField.placeholderString = "Find in page"
        searchField.sendsSearchStringImmediately = true
        searchField.sendsWholeSearchString = false
        searchField.delegate = self

        statusLabel.font = .systemFont(ofSize: 11)
        statusLabel.textColor = .secondaryLabelColor

        func configure(_ btn: NSButton, symbol: String, label: String, action: Selector) {
            btn.image = NSImage(systemSymbolName: symbol, accessibilityDescription: label)
            btn.target = self
            btn.action = action
            btn.bezelStyle = .accessoryBarAction
            btn.isBordered = false
        }
        configure(prevButton, symbol: "chevron.left",     label: "Previous", action: #selector(prevTapped))
        configure(nextButton, symbol: "chevron.right",    label: "Next",     action: #selector(nextTapped))
        configure(closeButton, symbol: "xmark.circle.fill", label: "Close",  action: #selector(closeTapped))

        divider.boxType = .separator

        for v in [searchField, statusLabel, prevButton, nextButton, closeButton, divider] as [NSView] {
            v.translatesAutoresizingMaskIntoConstraints = true  // frame-based, NOT Auto Layout
            addSubview(v)
        }
        autoresizesSubviews = false  // we manage all subview frames in layout()
    }

    /// All layout done manually in frame space — Auto Layout in here made the
    /// window collapse to 1 px on first display.
    override func layout() {
        super.layout()
        let pad: CGFloat = 10
        let h = bounds.height
        let btn: CGFloat = 22
        let btnY = (h - btn) / 2
        let searchH: CGFloat = 22
        let searchY = (h - searchH) / 2

        // Right group, right-aligned.
        let closeX = bounds.width - pad - btn
        closeButton.frame = NSRect(x: closeX, y: btnY, width: btn, height: btn)
        let nextX = closeX - 6 - btn
        nextButton.frame = NSRect(x: nextX, y: btnY, width: btn, height: btn)
        let prevX = nextX - 2 - btn
        prevButton.frame = NSRect(x: prevX, y: btnY, width: btn, height: btn)

        // Search field left.
        let searchLeft: CGFloat = 12
        let searchMax: CGFloat = min(320, prevX - 12 - searchLeft)
        let searchWidth = max(80, searchMax)
        searchField.frame = NSRect(x: searchLeft, y: searchY, width: searchWidth, height: searchH)

        // Status label between search field and prev button.
        let statusX = searchField.frame.maxX + 8
        let statusW = max(0, prevX - 8 - statusX)
        statusLabel.frame = NSRect(x: statusX, y: 0, width: statusW, height: h)
        statusLabel.alignment = .left

        // Divider at the bottom edge.
        divider.frame = NSRect(x: 0, y: 0, width: bounds.width, height: 1)
    }

    @objc private func prevTapped() { delegate?.findBarPreviousRequested() }
    @objc private func nextTapped() { delegate?.findBarNextRequested() }
    @objc private func closeTapped() { delegate?.findBarCloseRequested() }

    func controlTextDidChange(_ obj: Notification) {
        delegate?.findBarSearchTextChanged(searchField.stringValue)
    }

    func control(_ control: NSControl, textView: NSTextView, doCommandBy commandSelector: Selector) -> Bool {
        if commandSelector == #selector(NSResponder.insertNewline(_:)) {
            if NSEvent.modifierFlags.contains(.shift) { delegate?.findBarPreviousRequested() }
            else                                      { delegate?.findBarNextRequested() }
            return true
        }
        if commandSelector == #selector(NSResponder.cancelOperation(_:)) {
            delegate?.findBarCloseRequested()
            return true
        }
        return false
    }
}
