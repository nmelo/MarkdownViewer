//
//  Settings.swift
//  MarkdownViewer
//
//  Created by Sbarex on 13/12/20.
//

import Foundation
import OSLog

enum CMARK_Error: Error {
    case parser_create
    case parser_parse
}

enum Appearance: Int {
    case undefined
    case light
    case dark
}

enum JSExtension: Codable {
    enum CodingKeys: String, CodingKey {
        case state
        case url
    }
    
    case disabled
    case embed(url: URL?)
    case link(url: URL?)
    
    init(from decoder: any Decoder) throws {
        let container = try decoder.container(keyedBy: CodingKeys.self)
        
        let state = try container.decode(Int.self, forKey: .state)
        if state == 0 {
            self = .disabled
        } else {
            let url = try container.decode(URL?.self, forKey: .url)
            if state == 1 {
                self = .embed(url: url)
            } else {
                self = .link(url: url)
            }
        }
    }
    
    func encode(to encoder: any Encoder) throws {
        var container = encoder.container(keyedBy: CodingKeys.self)
        
        switch self {
        case .disabled:
            try container.encode(0, forKey: .state)
        case .embed(let url):
            try container.encode(1, forKey: .state)
            try container.encode(url, forKey: .url)
        case .link(let url):
            try container.encode(2, forKey: .state)
            try container.encode(url, forKey: .url)
        }
    }
    
    var isEnabled: Bool {
        return !self.isDisabled
    }
    
    var isDisabled: Bool {
        switch self {
        case .disabled:
            return true
        default:
            return false
        }
    }
    
    /**
     * Sanitize the settings
     * - parameters:
     *   - cacheUrl: Path (local file or web uRL) of the library, from the cache folder or the main bundle.
     *   - cdnUrl: Web url from download the library. Tipically from a CDN service.
     *   - allowLinkFile: `true` allows you to link the library even if it is a local file and not a web URL.
     *
     * You can embed only exists local file. 
     */
    public mutating func sanitize(cacheUrl: URL?, cdnUrl: URL, allowLinkFile: Bool = false) {
        switch self {
        case .disabled:
            break
        case .link(let url):
            if let url = url ?? cacheUrl, allowLinkFile || !url.isFileURL {
                // Without `allowLinkFile`, only web url can be linked.
                // For link do not test if the file exists.
                self = .link(url: url)
            } else {
                // Link the CDN url.
                self = .link(url: cdnUrl)
            }
        case .embed(let url):
            if let url = url ?? cacheUrl {
                if url.isFileURL && FileManager.default.fileExists(atPath: url.path) {
                    // Only exists file can be embed.
                    self = .embed(url: url)
                } else if !url.isFileURL {
                    // Link a web url.
                    self = .link(url: cacheUrl)
                } else {
                    // Link the CDN url.
                    self = .link(url: cdnUrl)
                }
            } else {
                // Link the CDN url.
                self = .link(url: cdnUrl)
            }
        }
    }
    
    /**
     * Get the code to link/embed the JS library.
     * - parameters:
     *  - extraTagLink: Extra code to put in the `<script>` tag when the library is linked.
     *  - extraTagEmbed: Extra code to put in the `<script>` tag when the library is embedded.
     *
     * **Call `sanitize` before invokint this function.**
     */
    func getScriptCode(extraTagLink: String = "", extraTagEmbed: String = "") -> String {
        switch self {
        case .disabled:
            return ""
        case .link(let url):
            guard let url else {
                return ""
            }
            return "<script type='text/javascript' \(extraTagLink) src='\(url.absoluteString)'></script>\n"
        case .embed(let url):
            guard let url else {
                return ""
            }
            if let code = try? String(contentsOfFile: url.path, encoding: .utf8) {
                // Embed the libraty inline
                return "<script type='text/javascript' \(extraTagEmbed)>\n\(code)\n</script>\n"
            }
            return Self.link(url: url).getScriptCode(extraTagLink: extraTagLink, extraTagEmbed: extraTagEmbed)
        }
    }
}

enum YamlMode: Int, Codable {
    case disabled = 0
    case allFiles = 1
    case onlyRmd = 2
}

enum EmojiMode: Int, Codable {
    case disabled = 0
    case font = 1
    case images = 2
}

enum StrikethroughMode: Int, Codable {
    case disabled = 0
    case single = 1
    case double = 2
}

extension NSNotification.Name {
    public static let MarkdownViewerSettingsUpdated: NSNotification.Name = NSNotification.Name("org.sbarex.markdownviewer-settings-changed")
}

// MARK: -
class Settings: Codable {
    enum CodingKeys: String, CodingKey {
        case autoLinkExtension
        case checkboxExtension
        case headsExtension
        case hightlightExtension
        case inlineImageExtension
        case mathExtension
        case mermaidExtension
        case mentionExtension
        case subExtension
        case supExtension
        case tableExtension
        case tagFilterExtension
        case taskListExtension
        case yamlExtension
        case emojiExtension
        case strikethroughExtension
        case syntaxHighlightExtension
        case syntaxWordWrapOption
        case syntaxLineNumbersOption
        case syntaxTabsOption
        case footnotesOption
        case hardBreakOption
        case noSoftBreakOption
        case unsafeHTMLOption
        case smartQuotesOption
        case validateUTFOption
        case baseFontSize
        case customCSS
        case customCSSCode
        case customCSSCodeFetched
        case customCSSOverride
        case openInlineLink
        case renderAsCode
        case debug
    }

    // MARK: - Static properties and methods
    
    /// Shared instance of the Settings.
    static let shared = {
        return Settings()
    }()
    
    /// URL of the Application Bundle.
    static var appBundleUrl: URL?
    
    /**
     * Get the Bundle with the resources.
     * For the host app return the main Bundle. For the appex return the bundle of the hosting app.
     */
    static func getResourceBundle() -> Bundle {
        if let url = Settings.appBundleUrl, let appBundle = Bundle(url: url) {
            return appBundle
        } else if let url = Settings.appBundleUrl?.appendingPathComponent("Contents/Resources"), let appBundle = Bundle(url: url) {
            return appBundle
        } else if Bundle.main.bundlePath.hasSuffix(".appex") {
            // this is an app extension
            let url = Bundle.main.bundleURL.deletingLastPathComponent().deletingLastPathComponent()

            if let appBundle = Bundle(url: url) {
                return appBundle
            } else if let appBundle = Bundle(identifier: "org.sbarex.MarkdownViewer") {
                return appBundle
            }
            // To access the main bundle, the extension must not be sandboxed (or must have a security exception entitlement to access the entire disk).
            os_log(
                "Unable to open the main application bundle from %{public}@",
                log: OSLog.quickLookExtension,
                type: .error,
                url.path
            )
            if let appBundle = Bundle(url: Bundle.main.bundleURL.appendingPathComponent("Contents/Resources")) {
                return appBundle
            } else if let appBundle = Bundle(url: Bundle.main.bundleURL) {
                return appBundle
            }
        }
        
        return Bundle.main
    }
    
    static var isLightAppearance: Bool {
        get {
            return UserDefaults.standard.string(forKey: "AppleInterfaceStyle") ?? "Light" == "Light"
        }
    }
    
    /// URL of the Application Support folder.
    /// Upstream used `containerURL(forSecurityApplicationGroupIdentifier:)`
    /// because the Quick Look extension and the host app shared state via an
    /// app group. We don't have an extension and we don't ship the group
    /// entitlement; calling that API on macOS triggers the App Management TCC
    /// consent ("would like to access data from other apps") on every open.
    /// Use the standard per-user Application Support directory instead.
    class var applicationSupportUrl: URL? {
        return FileManager.default.urls(for: .applicationSupportDirectory, in: .userDomainMask)
            .first?
            .appendingPathComponent("MarkdownViewer")
    }
    
    /**
     * URL of the folder for the style sheets.
     * * SeeAlso
     * Settings.applicationSupportUrl
     */
    static var jsFolder: URL? {
        return Settings.applicationSupportUrl?.appendingPathComponent("js")
    }
    
    /**
     * Informative hidden message.
     */
    static var aboutComment: String {
        var title: String = "<!--\n\nFile generated with MarkdownViewer [https://github.com/sbarex/QLMarkdown] - ";
        if let info = Bundle.main.infoDictionary {
            title += (info["CFBundleExecutable"] as? String ?? "MarkdownViewer")
            if let version = info["CFBundleShortVersionString"] as? String,
                let build = info["CFBundleVersion"] as? String {
                title += ", version \(version) (\(build))"
            }
            if let copy = info["NSHumanReadableCopyright"] as? String {
                title += ".\n\(copy.trimmingCharacters(in: CharacterSet(charactersIn: ". ")) + " with ❤️")"
            }
        }
        title += "\n\n-->\n"
        return title
    }
    
    // MARK: - Instance properties and methods
    
    var autoLinkExtension: Bool = true
    var checkboxExtension: Bool = false
    var headsExtension: Bool = true
    var highlightExtension: Bool = false
    var inlineImageExtension: Bool = true
    var mathExtension: JSExtension = .link(url: nil)
    var mermaidExtension: JSExtension = .link(url: nil)
    var mentionExtension: Bool = false
    var subExtension: Bool = false
    var supExtension: Bool = false
    var tableExtension: Bool = true
    var tagFilterExtension: Bool = true
    var taskListExtension: Bool = true
    var yamlExtension: YamlMode = .onlyRmd
    var emojiExtension: EmojiMode = .font
    var strikethroughExtension: StrikethroughMode = .single
    var syntaxHighlightExtension: Bool = true
    var syntaxWordWrapOption: Int = 0
    var syntaxLineNumbersOption: Bool = false
    var syntaxTabsOption: Int = 4

    var footnotesOption: Bool = true
    var hardBreakOption: Bool = false
    var noSoftBreakOption: Bool = false
    var unsafeHTMLOption: Bool = true
    var smartQuotesOption: Bool = true
    var validateUTFOption: Bool = false
    
    var baseFontSize: CGFloat = 0
    var customCSS: URL? {
        didSet {
            customCSSFetched = false
        }
    }
    var customCSSFetched: Bool = false
    var customCSSCode: String?
    var customCSSOverride: Bool = false
    
    var openInlineLink: Bool = false
    var renderAsCode: Bool = false

    /// Show debug infomations.
    var debug: Bool = false
    
    lazy fileprivate(set) var resourceBundle: Bundle = {
        return Self.getResourceBundle()
    }()
    
    required init(from decoder: Decoder) throws {
        let container = try decoder.container(keyedBy: CodingKeys.self)
        
        self.tableExtension = try container.decode(Bool.self, forKey: .tableExtension)
        self.autoLinkExtension = try container.decode(Bool.self, forKey:.autoLinkExtension)
        self.tagFilterExtension = try container.decode(Bool.self, forKey: .tagFilterExtension)
        self.taskListExtension = try container.decode(Bool.self, forKey: .taskListExtension)
        
        self.yamlExtension = try container.decode(YamlMode.self, forKey: .yamlExtension)
    
        self.strikethroughExtension = try container.decode(StrikethroughMode.self, forKey:.strikethroughExtension)
        
        self.mathExtension = try container.decode(JSExtension.self, forKey:.mathExtension)
        self.mermaidExtension = try container.decode(JSExtension.self, forKey:.mermaidExtension)
        
        self.mentionExtension = try container.decode(Bool.self, forKey:.mentionExtension)
        self.checkboxExtension = try container.decode(Bool.self, forKey:.checkboxExtension)
        self.headsExtension = try container.decode(Bool.self, forKey:.headsExtension)
        self.highlightExtension = try container.decode(Bool.self, forKey: .hightlightExtension)
       
        self.syntaxHighlightExtension = try container.decode(Bool.self, forKey: .syntaxHighlightExtension)
        self.syntaxWordWrapOption = try container.decode(Int.self, forKey: .syntaxWordWrapOption)
        self.syntaxLineNumbersOption = try container.decode(Bool.self, forKey: .syntaxLineNumbersOption)
        self.syntaxTabsOption = try container.decode(Int.self, forKey: .syntaxTabsOption)
        
        self.subExtension = try container.decode(Bool.self, forKey:.subExtension)
        self.supExtension = try container.decode(Bool.self, forKey:.supExtension)
        
        self.emojiExtension = try container.decode(EmojiMode.self, forKey:.emojiExtension)
        
        self.inlineImageExtension = try container.decode(Bool.self, forKey:.inlineImageExtension)
        
        self.hardBreakOption = try container.decode(Bool.self, forKey: .hardBreakOption)
        self.noSoftBreakOption = try container.decode(Bool.self, forKey: .noSoftBreakOption)
        self.unsafeHTMLOption = try container.decode(Bool.self, forKey: .unsafeHTMLOption)
        self.validateUTFOption = try container.decode(Bool.self, forKey: .validateUTFOption)
        self.smartQuotesOption = try container.decode(Bool.self, forKey: .smartQuotesOption)
        self.footnotesOption = try container.decode(Bool.self, forKey: .footnotesOption)
        
        self.baseFontSize = try container.decode(CGFloat.self, forKey: .baseFontSize)
        self.customCSS = try container.decode(URL?.self, forKey: .customCSS)
        self.customCSSFetched = try container.decode(Bool.self, forKey: .customCSSCodeFetched)
        self.customCSSCode = try container.decode(String?.self, forKey: .customCSSCode)
        self.customCSSOverride = try container.decode(Bool.self, forKey: .customCSSOverride)
        
        self.debug = try container.decode(Bool.self, forKey: .debug)

        self.openInlineLink = try container.decode(Bool.self, forKey: .openInlineLink)
        self.renderAsCode = try container.decode(Bool.self, forKey: .renderAsCode)
    }
    
    init() { }


    func encode(to encoder: Encoder) throws {
        var container = encoder.container(keyedBy: CodingKeys.self)
        
        try container.encode(self.tableExtension, forKey: .tableExtension)
        try container.encode(self.autoLinkExtension, forKey: .autoLinkExtension)
        try container.encode(self.tagFilterExtension, forKey: .tagFilterExtension)
        try container.encode(self.taskListExtension, forKey: .taskListExtension)
    
        try container.encode(self.yamlExtension, forKey: .yamlExtension)
    
        try container.encode(self.strikethroughExtension, forKey: .strikethroughExtension)
        
        try container.encode(self.mathExtension, forKey: .mathExtension)
        try container.encode(self.mermaidExtension, forKey: .mermaidExtension)
        
        try container.encode(self.mentionExtension, forKey: .mentionExtension)
        try container.encode(self.checkboxExtension, forKey: .checkboxExtension)
        try container.encode(self.headsExtension, forKey: .headsExtension)
        try container.encode(self.highlightExtension, forKey: .hightlightExtension)
        
        try container.encode(self.syntaxHighlightExtension, forKey: .syntaxHighlightExtension)
        try container.encode(self.syntaxWordWrapOption, forKey: .syntaxWordWrapOption)
        try container.encode(self.syntaxLineNumbersOption, forKey: .syntaxLineNumbersOption)
        try container.encode(self.syntaxTabsOption, forKey: .syntaxTabsOption)
        
        try container.encode(self.subExtension, forKey: .subExtension)
        try container.encode(self.supExtension, forKey: .supExtension)
        
        try container.encode(self.emojiExtension, forKey: .emojiExtension)
        
        try container.encode(self.inlineImageExtension, forKey: .inlineImageExtension)
    
        try container.encode(self.hardBreakOption, forKey: .hardBreakOption)
        try container.encode(self.noSoftBreakOption, forKey: .noSoftBreakOption)
        try container.encode(self.unsafeHTMLOption, forKey: .unsafeHTMLOption)
        try container.encode(self.validateUTFOption, forKey: .validateUTFOption)
        try container.encode(self.smartQuotesOption, forKey: .smartQuotesOption)
        try container.encode(self.footnotesOption, forKey: .footnotesOption)
        
        try container.encode(self.baseFontSize, forKey: .baseFontSize)
        try container.encode(self.customCSS, forKey: .customCSS)
        try container.encode(self.customCSSCode, forKey: .customCSSCode)
        try container.encode(self.customCSSFetched, forKey: .customCSSCodeFetched)
        try container.encode(self.customCSSOverride, forKey: .customCSSOverride)
        
        try container.encode(self.debug, forKey: .debug)

        try container.encode(self.openInlineLink, forKey: .openInlineLink)
        try container.encode(self.renderAsCode, forKey: .renderAsCode)
    }
    
    /**
     * Get the contents of a file insie dhe Reource Bundle.
     *  - parameters:
     *    - name: Name of the resource.
     *    - ext: Extension of the resource
     */
    func getBundleContents(forResource name: String, ofType ext: String) -> String? {
        if let p = self.resourceBundle.path(forResource: name, ofType: ext), let data = FileManager.default.contents(atPath: p), let s = String(data: data, encoding: .utf8) {
            return s
        } else {
            return nil
        }
    }
    
    /**
     * Get the custom CSS code
     */
    func getCustomCSSCode() -> String? {
        guard let url = self.customCSS, url.lastPathComponent != "-" else {
            return nil
        }
        return try? String(contentsOf: url, encoding: .utf8)
    }
    
    /**
     * Install the dependencies files.
     *
     * This function create the support folders and copy from the bundle, if available, the mermaid and mathjax libraries.
     * Then copy the support files of highlight.
     */
    func installDependencies(override: Bool = false) {
        try? installDep(forResource: "mermaid.min", withExtension: "js", to: Self.mermaidCacheFileUrl, overwrite: override)
        try? installDep(forResource: "tex-mml-chtml", withExtension: "js", to: Self.mathJaxCacheFileUrl, overwrite: override)
        
        try? installDep(forResource: "highlight", withExtension: nil, to: Settings.syntaxHighlightSupportCacheUrl, overwrite: override)
    }
    
    private func installDep(forResource name: String, withExtension ext: String?, to destination: URL?, overwrite: Bool) throws {
        guard let source = self.resourceBundle.url(forResource: name, withExtension: ext) else {
            os_log(
                "Unable to store cache the file/folder %{public}s: source is missing on the app bundle!",
                log: OSLog.quickLookExtension,
                type: .error,
                "\(name)\(ext != nil ? "." + ext! : "")"
            )
            return
        }
        
        do {
            try installDep(from: source, to: destination, overwrite: overwrite)
        } catch {
            os_log(
                "Unable to store cache the file/folder %{public}s to %{public}s: %{public}s!",
                log: OSLog.quickLookExtension,
                type: .error,
                "\(name)\(ext != nil ? "." + ext! : "")",
                destination?.path ?? "N/D",
                error.localizedDescription
            )
            throw error
        }
    }
    
    private func installDep(from source: URL?, to destination: URL?, overwrite: Bool) throws {
        guard let source, let destination else {
            return
        }
        let fileManager = FileManager.default
        let exists = fileManager.fileExists(atPath: destination.path)
        guard overwrite || !exists else {
            return
        }
        if exists {
            try fileManager.removeItem(at: destination)
        }
        let folder = destination.deletingLastPathComponent()
            
        if !fileManager.fileExists(atPath: folder.path) {
            try fileManager.createDirectory(at: folder, withIntermediateDirectories: true, attributes: nil)
        }
        
        try fileManager.copyItem(atPath: source.path, toPath: destination.path)
    }
    
    /**
     * Download and cache a fiile from web.
     * - parameters:
     *   - source: Source url.
     *   - destination: Destination path
     *   - reply: Action to perform after the download.
     */
    static func fetchCacheFile(from source: URL, to destination: URL, withReply reply: ((Bool) -> Void)?) {
        let cacheFolderUrl = destination.deletingLastPathComponent()
        
        let fileManager = FileManager.default
        if !fileManager.fileExists(atPath: cacheFolderUrl.path) {
            do {
                try FileManager.default.createDirectory(at: cacheFolderUrl, withIntermediateDirectories: true, attributes: nil)
            } catch {
                reply?(false)
                return
            }
        }
        
        let task = URLSession.shared.downloadTask(with: source) { tempURL, response, error in
            if let error = error {
                print("Unable to fetch \(source.absoluteString):", error)
                os_log("Unable to fetch %{public}s", log: OSLog.rendering, type: .error, source.absoluteString)
                reply?(false)
                return
            }
            
            guard let tempURL = tempURL else {
                print("No file downloaded")
                os_log("No file downloaded from %{public}s", log: OSLog.rendering, type: .error, source.absoluteString)
                reply?(false)
                return
            }
            
            do {
                // Rimuove se esiste già
                if fileManager.fileExists(atPath: destination.path) {
                    try fileManager.removeItem(at: destination)
                }
                
                // Sposta il file temporaneo
                try fileManager.moveItem(at: tempURL, to: destination)
                
                // print("File seved in:", mermaidCacheFileUrl)
                reply?(true)
            } catch {
                print("Error storing file on \(destination.path):", error)
                os_log("Error storing mermaid file on %{public}s: %{public}s", log: OSLog.rendering, type: .error, destination.path, error.localizedDescription)
                reply?(false)
            }
        }
        
        task.resume()
    }
    
}

// MARK: - Mermaid support
extension Settings {
    /// Url from which to download the mermaid library.
    /// Mermaid v11+ no longer ships a UMD bundle that exposes `window.mermaid`
    /// from `mermaid.min.js` — that file is now an esbuild ESM wrapper. Use
    /// the explicit `.esm.min.mjs` and load it via `<script type="module">`.
    static let mermaidWebUrl = URL(string: "https://cdn.jsdelivr.net/npm/mermaid@11/dist/mermaid.esm.min.mjs")!
    
    /// Local file with the mermaid library.
    static var mermaidCacheFileUrl: URL? {
        return Self.jsFolder?.appendingPathComponent("mermaid.min.js")
    }
    
    /// Location of the mermaid library. Can be from the file cache or from the bundle.
    var mermaidFileUrl: URL? {
        return Self.mermaidCacheFileUrl ?? self.resourceBundle.url(forResource: "mermaid.min", withExtension: "js")
    }
}

// MARK: - MathJax
extension Settings {
    /// Url from which to download the mermaid library.
    static let mathJaxWebUrl = URL(string: "https://cdn.jsdelivr.net/npm/mathjax/es5/tex-mml-chtml.js")!
    
    /// Cache of the mermaid library.
    static var mathJaxCacheFileUrl: URL? {
        return Self.jsFolder?.appendingPathComponent("tex-mml-chtml.js")
    }
    
    /// Location of the mermaid library. Can be from the file cache or from the bundle.
    var mathJaxFileUrl: URL? {
        return Self.mathJaxCacheFileUrl ?? self.resourceBundle.url(forResource: "tex-mml-chtml", withExtension: "js")
    }
}

// MARK: - Syntax highlight
extension Settings {
    /// Url from which to download the `highlight` support files.
    static var syntaxHighlightSupportCacheUrl: URL? {
        return Self.applicationSupportUrl?.appendingPathComponent("highlight")
    }
    
    /// Get the path of folder with `highlight` support files.
    func getHighlightSupportPath() -> String? {
        if let cache = Self.syntaxHighlightSupportCacheUrl, FileManager.default.fileExists(atPath: cache.path) {
            return cache.path
        }
        
        return self.resourceBundle.url(forResource: "highlight", withExtension: "")?.path
    }
}
