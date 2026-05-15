//
//  Settings.swift
//  MarkdownViewer
//
//  Created by Sbarex on 25/12/20.
//

import Foundation
import AppKit

// MARK: -
extension Settings {
    /**
     * Check if the url is of type file and if it exists.
     * - parameters:
     *   - url: Url from fetch the librarty. If not set uses the `mathJaxFileUrl`.
     */
    public func allowToEmbedMathJax(customUrl url: URL? = nil) -> Bool {
        guard let library = url ?? mathJaxFileUrl else {
            return false
        }
        return library.isFileURL && FileManager.default.fileExists(atPath: library.path)
    }
    
    /**
     * Check if the url is of not a file.
     * - parameters:
     *   - url: Url from fetch the librarty. If not set uses the `mathJaxFileUrl`.
     */
    public func allowToLinkMathJax(customUrl url: URL? = nil) -> Bool {
        guard let library = url ?? mathJaxFileUrl else {
            return false
        }
        return !library.isFileURL
    }
    
    public func getMathJaxFileSize() -> Int
    {
        return  (try? self.mathJaxFileUrl?.resourceValues(forKeys: [.fileSizeKey]))?.fileSize ?? 0
    }
    
    /// Download and cache the mermaid library from web.
    func updateMathJaxUCache(_ reply: ((Bool) -> Void)?) {
        guard let mathJaxCacheFileUrl = Self.mathJaxCacheFileUrl else {
            reply?(false)
            return
        }
        Self.fetchCacheFile(from: Self.mathJaxWebUrl, to: mathJaxCacheFileUrl, withReply: reply)
    }
    
    /// Download and cache the mermaid library from web.
    func updateMemaidCache(_ reply: ((Bool) -> Void)?) {
        guard let mermaidCacheFileUrl = Self.mermaidCacheFileUrl else {
            reply?(false)
            return
        }
        Self.fetchCacheFile(from: Self.mermaidWebUrl, to: mermaidCacheFileUrl, withReply: reply)
    }
    
    /**
     * Check if the url is of type file and if it exists.
     * - parameters:
     *   - url: Url from fetch the librarty. If not set uses the `mermaidFileUrl`.
     */
    public func allowToEmbedMermaid(customUrl url: URL? = nil) -> Bool {
        guard let library = url ?? mermaidFileUrl else {
            return false
        }
        return library.isFileURL && FileManager.default.fileExists(atPath: library.path)
    }
    
    /**
     * Check if the url is of not a file.
     * - parameters:
     *   - url: Url from fetch the librarty. If not set uses the `mermaidUrl`.
     */
    public func allowToLinkMermaid(customUrl url: URL? = nil) -> Bool {
        guard let library = url ?? mermaidFileUrl else {
            return false
        }
        return !library.isFileURL
    }
    
    public func getMermaidFileSize() -> Int
    {
        return  (try? self.mermaidFileUrl?.resourceValues(forKeys: [.fileSizeKey]))?.fileSize ?? 0
    }
    
}
