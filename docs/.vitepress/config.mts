import { defineConfig } from "vitepress";

export default defineConfig({
  title: "Arc",
  description: "Deterministic ESP32-S3 firmware substrate on ESP-IDF.",
  base: process.env.ARC_DOCS_BASE ?? "/",
  cleanUrls: true,
  lastUpdated: true,
  themeConfig: {
    siteTitle: "Arc",
    nav: [
      { text: "Guide", link: "/getting-started" },
      { text: "Integrate", link: "/production-integration" },
      { text: "Modules", link: "/modules" },
      { text: "API", link: "/api" },
      { text: "Examples", link: "/examples" },
      { text: "Security", link: "/security" },
      { text: "License", link: "/licensing" },
      { text: "Benchmarks", link: "/benchmarking" },
      { text: "Safety", link: "/safety-case" },
      { text: "References", link: "/references" }
    ],
    sidebar: [
      {
        text: "Start",
        items: [
          { text: "Overview", link: "/" },
          { text: "Getting Started", link: "/getting-started" },
          { text: "Architecture", link: "/architecture" },
          { text: "Safety Patterns", link: "/safety-patterns" },
          { text: "Production Integration", link: "/production-integration" },
          { text: "Troubleshooting", link: "/troubleshooting" },
          { text: "Glossary", link: "/glossary" },
          { text: "Module Guide", link: "/modules" },
          { text: "Module Page Index", link: "/module-pages" },
          { text: "API Reference", link: "/api" },
          { text: "Examples", link: "/examples" },
          { text: "Security Automation", link: "/security" },
          { text: "API Naming", link: "/api-naming" }
        ]
      },
      {
        text: "Evidence",
        items: [
          { text: "Benchmarking", link: "/benchmarking" },
          { text: "Safety Case", link: "/safety-case" },
          { text: "HIL Test Jig", link: "/hil-test-jig" },
          { text: "Licensing", link: "/licensing" },
          { text: "References", link: "/references" }
        ]
      }
    ],
    search: {
      provider: "local"
    },
    editLink: {
      pattern: "https://github.com/dhimasardinata/arc/edit/main/docs/:path",
      text: "Edit this page on GitHub"
    },
    socialLinks: [{ icon: "github", link: "https://github.com/dhimasardinata/arc" }],
    footer: {
      message: "ESP32-S3 first. ESP-IDF native. Static ownership by default.",
      copyright: "Released under AGPL-3.0-only."
    }
  }
});
