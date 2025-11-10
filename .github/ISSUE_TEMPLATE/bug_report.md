name: Bug Report
description: Something isn't working quite right in the software.
labels: [bug]
body:
- type: markdown
  attributes:
    value: |
      Bug reports should only be used for reporting issues with how the software works. For assistance installing this software, as well as debugging issues with dependencies, please use our Discord Server.

- type: textarea
  attributes:
    label: Current Behavior
    description: Please provide a clear & concise description of the issue.
  validations:
    required: true

- type: textarea
  attributes:
    label: Expected Behavior
    description: Please describe what you expected to happen.
  validations:
    required: true

