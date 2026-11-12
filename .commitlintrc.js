module.exports = {
  extends: ["@commitlint/config-conventional"],
  rules: {
    // Set to warning level since Dependabot creates long commit message lines
    // that we have no control.
    "header-max-length": [1, "always", 100],
    "body-max-line-length": [1, "always", 100],
    // Squash-merge concatenates commit bodies, and every line after a
    // `Fixes #N` reference is parsed as footer, so relax this too.
    "footer-max-line-length": [1, "always", 100],
  },
};
