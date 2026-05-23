//
//  ViewController.swift
//  MarkdownViewer
//
//  Standalone viewer: renders a markdown file and live-reloads on change.
//  The window is a split view: a sidebar with a clickable heading index
//  (the "TOC") on either the left or the right, and the rendered document
//  in the main pane.
//

import Cocoa
@preconcurrency import WebKit

// MARK: - TOC data

struct TOCHeading {
    let level: Int
    let text: String
    let id: String
}

extension Notification.Name {
    static let tocSettingsChanged = Notification.Name("MarkdownViewer.TOCSettingsChanged")
}

// MARK: - Container split controller

class ViewController: NSSplitViewController {

    static let tocVisibleKey = "TOCVisible"
    static let tocOnRightKey = "TOCOnRight"

    static var tocVisible: Bool {
        get { (UserDefaults.standard.object(forKey: tocVisibleKey) as? Bool) ?? true }
        set { UserDefaults.standard.set(newValue, forKey: tocVisibleKey) }
    }
    static var tocOnRight: Bool {
        get { UserDefaults.standard.bool(forKey: tocOnRightKey) }
        set { UserDefaults.standard.set(newValue, forKey: tocOnRightKey) }
    }

    let contentVC = ContentViewController()
    private let tocVC = TOCSidebarController()
    private var tocItem: NSSplitViewItem!
    private var contentItem: NSSplitViewItem!

    /// Exposed for AppDelegate's "is this window already showing this file?" lookup
    /// and "open in front-most empty window" reuse.
    var markdownFile: URL? { contentVC.markdownFile }

    override func viewDidLoad() {
        super.viewDidLoad()

        splitView.dividerStyle = .thin
        splitView.autosaveName = "MarkdownViewer.TOCSplit"

        // `sidebarWithViewController:` already sets behavior = .sidebar; the
        // property is get-only after construction.
        tocItem = NSSplitViewItem(sidebarWithViewController: tocVC)
        tocItem.canCollapse = true
        tocItem.minimumThickness = 180
        tocItem.maximumThickness = 480
        tocItem.preferredThicknessFraction = 0.22

        contentItem = NSSplitViewItem(viewController: contentVC)
        contentItem.minimumThickness = 320
        contentItem.canCollapse = false

        contentVC.onTOCUpdate = { [weak self] items in
            self?.tocVC.update(items: items)
        }
        tocVC.onSelect = { [weak self] heading in
            self?.contentVC.scrollTo(headingID: heading.id)
        }

        arrangeSplit(animated: false)

        NotificationCenter.default.addObserver(
            self,
            selector: #selector(externalTOCSettingsChanged(_:)),
            name: .tocSettingsChanged,
            object: nil
        )
    }

    deinit {
        NotificationCenter.default.removeObserver(self)
    }

    /// Rebuild the split arrangement from current settings. Re-orders items
    /// to flip left/right and updates the sidebar's collapsed state.
    private func arrangeSplit(animated: Bool) {
        let onRight = ViewController.tocOnRight
        let visible = ViewController.tocVisible

        let desired: [NSSplitViewItem] = onRight ? [contentItem, tocItem] : [tocItem, contentItem]
        if splitViewItems != desired {
            splitViewItems = desired
        }
        let target = !visible
        if animated {
            tocItem.animator().isCollapsed = target
        } else {
            tocItem.isCollapsed = target
        }
    }

    @objc private func externalTOCSettingsChanged(_ note: Notification) {
        // Skip our own broadcast — we already updated locally.
        if let sender = note.object as? ViewController, sender === self { return }
        arrangeSplit(animated: true)
    }

    private static func broadcastTOCSettingsChange(from sender: ViewController) {
        NotificationCenter.default.post(name: .tocSettingsChanged, object: sender)
    }

    // MARK: - Public API (preserved for AppDelegate)

    @discardableResult
    func openMarkdown(file: URL) -> Bool {
        return contentVC.openMarkdown(file: file)
    }

    @IBAction func reloadDocument(_ sender: Any?) {
        contentVC.reloadDocument(sender)
    }

    // MARK: - Forwarded menu actions

    @objc func performFind(_ sender: Any?)  { contentVC.performFind(sender) }
    @objc func findNext(_ sender: Any?)     { contentVC.findNext(sender) }
    @objc func findPrevious(_ sender: Any?) { contentVC.findPrevious(sender) }
    @objc func zoomIn(_ sender: Any?)       { contentVC.zoomIn(sender) }
    @objc func zoomOut(_ sender: Any?)      { contentVC.zoomOut(sender) }
    @objc func zoomReset(_ sender: Any?)    { contentVC.zoomReset(sender) }
    @objc func toggleWide(_ sender: Any?)   { contentVC.toggleWide(sender) }

    // MARK: - TOC menu actions

    @objc func toggleTOC(_ sender: Any?) {
        ViewController.tocVisible.toggle()
        arrangeSplit(animated: true)
        ViewController.broadcastTOCSettingsChange(from: self)
    }

    @objc func toggleTOCPosition(_ sender: Any?) {
        ViewController.tocOnRight.toggle()
        arrangeSplit(animated: true)
        ViewController.broadcastTOCSettingsChange(from: self)
    }

    // NSMenuItemValidation — called by AppKit before showing the menu.
    // (NSSplitViewController's superclasses don't declare this, so no `override`.)
    @objc func validateMenuItem(_ menuItem: NSMenuItem) -> Bool {
        switch menuItem.action {
        case #selector(toggleTOC(_:)):
            menuItem.title = ViewController.tocVisible ? "Hide Table of Contents" : "Show Table of Contents"
            return true
        case #selector(toggleTOCPosition(_:)):
            menuItem.title = ViewController.tocOnRight ? "Move Sidebar to Left" : "Move Sidebar to Right"
            return true
        default:
            return true
        }
    }
}

// MARK: - Content (webView + find bar + drop target)

final class ContentViewController: NSViewController {
    private var webView: WKWebView!
    private var findBar: FindBar!
    private var tocMessageProxy: WeakScriptMessageHandler!

    /// Called whenever a render finishes and the TOC has been (re)extracted.
    var onTOCUpdate: (([TOCHeading]) -> Void)?

    private(set) var markdownFile: URL? {
        didSet {
            // Changing files always means a full reload — invalidate the
            // "live document" marker so render() takes the full-load branch.
            if oldValue != markdownFile {
                loadedDocumentFile = nil
            }
            updateWindowTitle()
            render()
            restartFileWatch()
        }
    }

    private var fileSource: DispatchSourceFileSystemObject?
    private var reloadDebounce: DispatchWorkItem?
    private var pendingScrollRestore: CGFloat = 0

    /// File path that owns the currently-loaded document, set after the page
    /// finishes its first full load. `nil` means there's no live document we
    /// can update in place (empty state, error page, or different file).
    private var loadedDocumentFile: URL?

    override func loadView() {
        let root = NSView(frame: NSRect(x: 0, y: 0, width: 700, height: 700))
        root.autoresizingMask = [.width, .height]

        let config = WKWebViewConfiguration()
        let settings = Settings.shared
        config.preferences.javaScriptEnabled =
            (settings.unsafeHTMLOption && settings.inlineImageExtension)
            || !settings.mermaidExtension.isDisabled
            || !settings.mathExtension.isDisabled
        config.allowsAirPlayForMediaPlayback = false
        // Right-click → Inspect Element for debugging.
        config.preferences.setValue(true, forKey: "developerExtrasEnabled")

        // TOC extraction sends headings back from the page via this handler.
        // The proxy holds us weakly to avoid the WKWebView → config → ucc → handler → self cycle.
        let proxy = WeakScriptMessageHandler()
        self.tocMessageProxy = proxy
        config.userContentController.add(proxy, name: "toc")

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

        // Wire the message handler now that self is fully constructed.
        proxy.delegate = self

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

    // MARK: - TOC scroll

    /// Scroll the rendered page to the heading with the given id.
    func scrollTo(headingID: String) {
        let safe = headingID
            .replacingOccurrences(of: "\\", with: "\\\\")
            .replacingOccurrences(of: "'", with: "\\'")
        webView.evaluateJavaScript("""
        (function(){
            const el = document.getElementById('\(safe)');
            if (!el) return;
            el.scrollIntoView({behavior: 'smooth', block: 'start'});
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

        // Same file already fully loaded → swap the article body in place.
        // No blank-frame flicker, scroll position preserved natively.
        if loadedDocumentFile == file {
            renderInPlace(file: file)
        } else {
            performFullLoad(file: file)
        }
    }

    /// Initial / cross-file load: replaces the entire document. We capture
    /// the current scroll so we can restore it once the new page is ready.
    private func performFullLoad(file: URL) {
        loadedDocumentFile = nil  // suppress in-place updates until didFinish

        webView.evaluateJavaScript("document.documentElement.scrollTop || document.body.scrollTop || 0") { [weak self] result, _ in
            guard let self = self else { return }
            let scroll = (result as? CGFloat) ?? 0
            self.pendingScrollRestore = scroll
            self.doFullLoad(file: file)
        }
    }

    private func doFullLoad(file: URL) {
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

    /// Live-reload path: swap the contents of `<article>` on the already-loaded
    /// page. Avoids the blank-frame flicker that `loadHTMLString` causes when
    /// the file is being edited rapidly (e.g. an LLM streaming a doc).
    ///
    /// Content is parsed via `Range.createContextualFragment` then installed
    /// with `replaceChildren`, equivalent in trust posture to the existing
    /// `loadHTMLString` path — same `cmark-gfm`-rendered body, no extra
    /// untrusted input.
    private func renderInPlace(file: URL) {
        let settings = Settings.shared
        let resolved = Settings.getMarkdownFile(from: file)
        let appearance: Appearance = Settings.isLightAppearance ? .light : .dark

        let body: String
        do {
            body = try settings.render(
                file: resolved,
                forAppearance: appearance,
                baseDir: resolved.deletingLastPathComponent().path
            )
        } catch {
            // Rare for a file that previously rendered; fall back to the
            // full-load path so an error page can replace the document.
            performFullLoad(file: file)
            return
        }

        // Apply the same mermaid-block transform `getCompleteHTML` does.
        let processed = (!settings.renderAsCode && !settings.mermaidExtension.isDisabled && body.contains("language-mermaid"))
            ? settings.transformMermaidBlocks(body)
            : body

        let bodyLiteral = ContentViewController.jsStringLiteral(processed)
        let js = """
        (function(){
            const article = document.querySelector('article');
            if (!article) return false;
            // Parse the new markup as a DocumentFragment, then swap children.
            const wideClass = article.classList.contains('wide');
            const range = document.createRange();
            range.selectNodeContents(article);
            const frag = range.createContextualFragment(\(bodyLiteral));
            article.replaceChildren(frag);
            if (wideClass) article.classList.add('wide');
            // Re-typeset math in just the swapped subtree.
            if (window.MathJax && typeof window.MathJax.typesetPromise === 'function') {
                try { window.MathJax.typesetPromise([article]); } catch (e) {}
            }
            // Re-init any new mermaid diagrams (existing ones already carry data-processed).
            if (window.mermaid && typeof window.mermaid.run === 'function') {
                try { window.mermaid.run({ querySelector: 'article .mermaid:not([data-processed="true"])' }); } catch (e) {}
            }
            return true;
        })();
        """

        webView.evaluateJavaScript(js) { [weak self] result, _ in
            guard let self = self else { return }
            if (result as? Bool) == true {
                // Refresh the TOC from the freshly-swapped article.
                self.webView.evaluateJavaScript(ContentViewController.tocExtractorJS)
            } else {
                // No <article> on the page — page got replaced from under us.
                // Reload everything from scratch.
                self.performFullLoad(file: file)
            }
        }
    }

    /// JS-safe string literal — wraps a Swift string so it can be inlined
    /// inside a JavaScript expression. Uses `JSONSerialization` so any UTF-8,
    /// quotes, backslashes and control chars round-trip safely.
    fileprivate static func jsStringLiteral(_ s: String) -> String {
        if let data = try? JSONSerialization.data(withJSONObject: [s], options: []),
           let json = String(data: data, encoding: .utf8) {
            // JSONSerialization wraps in []; strip the brackets to get the bare literal.
            return String(json.dropFirst().dropLast())
        }
        // Hand-escaped fallback — shouldn't trigger for valid UTF-8 strings.
        let escaped = s
            .replacingOccurrences(of: "\\", with: "\\\\")
            .replacingOccurrences(of: "\"", with: "\\\"")
            .replacingOccurrences(of: "\n", with: "\\n")
            .replacingOccurrences(of: "\r", with: "\\r")
        return "\"\(escaped)\""
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
        // Empty doc — clear any prior TOC.
        onTOCUpdate?([])
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

    // MARK: - TOC extraction

    /// JS injected after every successful load. It assigns slug ids to any
    /// headings that don't already have one and posts the heading list back
    /// to Swift via `webkit.messageHandlers.toc`.
    fileprivate static let tocExtractorJS: String = """
    (function(){
        function slugify(s){
            return (s || '')
                .toLowerCase().trim()
                .replace(/[^\\p{L}\\p{N}\\s-]/gu, '')
                .replace(/\\s+/g, '-')
                .replace(/-+/g, '-')
                .replace(/^-|-$/g, '');
        }
        var hs = document.querySelectorAll('article h1, article h2, article h3, article h4, article h5, article h6');
        var seen = {};
        var out = [];
        for (var i = 0; i < hs.length; i++) {
            var h = hs[i];
            if (!h.id) {
                var base = slugify(h.textContent) || 'section';
                var id = base, n = 1;
                while (seen[id] || document.getElementById(id)) { n++; id = base + '-' + n; }
                h.id = id;
                seen[id] = true;
            } else {
                seen[h.id] = true;
            }
            out.push({
                level: parseInt(h.tagName.substring(1), 10),
                text: (h.textContent || '').trim(),
                id: h.id
            });
        }
        try { window.webkit.messageHandlers.toc.postMessage(out); } catch (e) {}
    })();
    """
}

// MARK: - Navigation + TOC reception

extension ContentViewController: WKNavigationDelegate, WKScriptMessageHandler {
    func webView(_ webView: WKWebView, didFinish navigation: WKNavigation!) {
        if pendingScrollRestore > 0 {
            let y = pendingScrollRestore
            pendingScrollRestore = 0
            webView.evaluateJavaScript("window.scrollTo(0, \(y));")
        }
        // The document is fully loaded — future reloads of the same file can
        // use the in-place article swap to avoid flicker.
        loadedDocumentFile = markdownFile
        webView.evaluateJavaScript(ContentViewController.tocExtractorJS)
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

    func userContentController(_ userContentController: WKUserContentController, didReceive message: WKScriptMessage) {
        guard message.name == "toc",
              let raw = message.body as? [[String: Any]] else { return }
        let items: [TOCHeading] = raw.compactMap { dict in
            guard let level = dict["level"] as? Int,
                  let text  = dict["text"]  as? String,
                  let id    = dict["id"]    as? String,
                  !text.isEmpty else { return nil }
            return TOCHeading(level: level, text: text, id: id)
        }
        onTOCUpdate?(items)
    }
}

extension ContentViewController: FindBarDelegate {
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

// MARK: - Weak proxy for WKScriptMessageHandler

/// `WKUserContentController.add(_:name:)` strongly retains its handler. Wrap
/// the real handler in this proxy and store `delegate` weakly so the
/// ContentViewController → WKWebView → ucc → handler chain doesn't keep
/// the view controller alive forever.
final class WeakScriptMessageHandler: NSObject, WKScriptMessageHandler {
    weak var delegate: WKScriptMessageHandler?
    func userContentController(_ userContentController: WKUserContentController,
                               didReceive message: WKScriptMessage) {
        delegate?.userContentController(userContentController, didReceive: message)
    }
}

// MARK: - TOC sidebar

final class TOCSidebarController: NSViewController {
    var onSelect: ((TOCHeading) -> Void)?

    private let outlineView = NSOutlineView()
    private let scrollView = NSScrollView()
    private let emptyLabel = NSTextField(labelWithString: "No headings")
    private var roots: [TOCNode] = []

    final class TOCNode {
        let heading: TOCHeading
        var children: [TOCNode] = []
        init(_ heading: TOCHeading) { self.heading = heading }
    }

    override func loadView() {
        let v = NSView(frame: NSRect(x: 0, y: 0, width: 240, height: 600))
        v.autoresizingMask = [.width, .height]

        scrollView.frame = v.bounds
        scrollView.autoresizingMask = [.width, .height]
        scrollView.hasVerticalScroller = true
        scrollView.hasHorizontalScroller = false
        scrollView.drawsBackground = false
        scrollView.borderType = .noBorder
        scrollView.automaticallyAdjustsContentInsets = true

        let col = NSTableColumn(identifier: NSUserInterfaceItemIdentifier("toc"))
        col.title = ""
        col.resizingMask = .autoresizingMask
        outlineView.addTableColumn(col)
        outlineView.outlineTableColumn = col
        outlineView.headerView = nil
        outlineView.indentationPerLevel = 12
        outlineView.indentationMarkerFollowsCell = true
        outlineView.rowSizeStyle = .small
        outlineView.style = .sourceList
        outlineView.allowsEmptySelection = true
        outlineView.allowsMultipleSelection = false
        outlineView.usesAutomaticRowHeights = true
        outlineView.dataSource = self
        outlineView.delegate = self
        outlineView.target = self
        outlineView.action = #selector(rowClicked(_:))
        outlineView.autoresizesOutlineColumn = true
        outlineView.backgroundColor = .clear

        scrollView.documentView = outlineView
        v.addSubview(scrollView)

        emptyLabel.translatesAutoresizingMaskIntoConstraints = false
        emptyLabel.font = .systemFont(ofSize: 11)
        emptyLabel.textColor = .secondaryLabelColor
        emptyLabel.alignment = .center
        emptyLabel.isHidden = true
        v.addSubview(emptyLabel)
        NSLayoutConstraint.activate([
            emptyLabel.centerXAnchor.constraint(equalTo: v.centerXAnchor),
            emptyLabel.topAnchor.constraint(equalTo: v.topAnchor, constant: 16),
            emptyLabel.leadingAnchor.constraint(greaterThanOrEqualTo: v.leadingAnchor, constant: 8),
            emptyLabel.trailingAnchor.constraint(lessThanOrEqualTo: v.trailingAnchor, constant: -8),
        ])

        view = v
    }

    func update(items headings: [TOCHeading]) {
        self.roots = TOCSidebarController.buildTree(from: headings)
        outlineView.reloadData()
        outlineView.expandItem(nil, expandChildren: true)
        emptyLabel.isHidden = !headings.isEmpty
    }

    /// Build a forest from a flat list of headings using the standard
    /// "previous heading is ancestor if its level is shallower" rule.
    private static func buildTree(from headings: [TOCHeading]) -> [TOCNode] {
        var roots: [TOCNode] = []
        var stack: [TOCNode] = []
        for h in headings {
            let node = TOCNode(h)
            while let top = stack.last, top.heading.level >= h.level {
                stack.removeLast()
            }
            if let parent = stack.last {
                parent.children.append(node)
            } else {
                roots.append(node)
            }
            stack.append(node)
        }
        return roots
    }

    @objc private func rowClicked(_ sender: Any?) {
        let row = outlineView.clickedRow
        guard row >= 0, let node = outlineView.item(atRow: row) as? TOCNode else { return }
        onSelect?(node.heading)
    }
}

extension TOCSidebarController: NSOutlineViewDataSource, NSOutlineViewDelegate {
    func outlineView(_ outlineView: NSOutlineView, numberOfChildrenOfItem item: Any?) -> Int {
        if let node = item as? TOCNode { return node.children.count }
        return roots.count
    }

    func outlineView(_ outlineView: NSOutlineView, child index: Int, ofItem item: Any?) -> Any {
        if let node = item as? TOCNode { return node.children[index] }
        return roots[index]
    }

    func outlineView(_ outlineView: NSOutlineView, isItemExpandable item: Any) -> Bool {
        return ((item as? TOCNode)?.children.isEmpty == false)
    }

    func outlineView(_ outlineView: NSOutlineView, viewFor tableColumn: NSTableColumn?, item: Any) -> NSView? {
        guard let node = item as? TOCNode else { return nil }
        let id = NSUserInterfaceItemIdentifier("toc.cell")
        let cell: NSTableCellView
        if let reused = outlineView.makeView(withIdentifier: id, owner: self) as? NSTableCellView {
            cell = reused
        } else {
            cell = NSTableCellView()
            cell.identifier = id
            let tf = NSTextField(labelWithString: "")
            tf.lineBreakMode = .byTruncatingTail
            tf.cell?.usesSingleLineMode = true
            tf.translatesAutoresizingMaskIntoConstraints = false
            tf.drawsBackground = false
            tf.isBordered = false
            tf.isEditable = false
            tf.font = .systemFont(ofSize: NSFont.smallSystemFontSize)
            cell.addSubview(tf)
            cell.textField = tf
            // Pin top + bottom (not just centerY) so the cell reports an
            // intrinsic height to NSOutlineView's automatic row sizing.
            NSLayoutConstraint.activate([
                tf.leadingAnchor.constraint(equalTo: cell.leadingAnchor, constant: 2),
                tf.trailingAnchor.constraint(equalTo: cell.trailingAnchor, constant: -4),
                tf.topAnchor.constraint(equalTo: cell.topAnchor, constant: 3),
                tf.bottomAnchor.constraint(equalTo: cell.bottomAnchor, constant: -3),
            ])
        }
        cell.textField?.stringValue = node.heading.text
        // Slightly de-emphasize deeper levels to mimic a TOC.
        let level = node.heading.level
        cell.textField?.textColor = (level <= 2) ? .labelColor : .secondaryLabelColor
        return cell
    }

    func outlineView(_ outlineView: NSOutlineView, shouldSelectItem item: Any) -> Bool { true }
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
