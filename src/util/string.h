
#pragma once

/**
 * @brief HTML-escape special characters in a string.
 *        Replaces & < > " with &amp; &lt; &gt; &quot;.
 * @param str  Input string (may be NULL).
 * @return malloc'd escaped string, caller must free.
 *         NULL input yields strdup("").
 */
char* str_escape(const char* str);

/**
 * @brief Resolver callback for str_template.
 * @param var  Variable name (without braces, e.g. "phone").
 * @param ctx  User-provided context pointer.
 * @return malloc'd value string (str_template will free it),
 *         or NULL if variable is unknown (kept as literal text).
 */
typedef char* (*str_template_resolver)(const char* var, void* ctx);

/**
 * @brief Simple template engine with {variable} substitution.
 *        Unknown variables are left as-is in the output.
 * @param tmpl    Template string, e.g. "From {phone}: {content}".
 * @param resolve Resolver callback, called for each {variable}.
 * @param ctx     User context passed through to resolve callback.
 * @return malloc'd formatted string, caller must free.
 */
char* str_template(const char* tmpl, str_template_resolver resolve, void* ctx);
