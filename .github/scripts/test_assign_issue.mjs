// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

import assert from 'node:assert/strict';
import { readFile } from 'node:fs/promises';
import test from 'node:test';

const workflowUrl = new URL('../workflows/assign-issue.yml', import.meta.url);
const workflow = await readFile(workflowUrl, 'utf8');
const workflowLines = workflow.split('\n');
const stepMarkerIndex = workflowLines.findIndex(
  (line) => line.trim() === '- name: Assign contributor',
);
assert.notEqual(
  stepMarkerIndex,
  -1,
  'expected assign-issue.yml to contain a step named "Assign contributor"; this suite extracts the workflow script by that literal step name',
);
const scriptMarkerIndex = workflowLines.findIndex(
  (line, index) => index > stepMarkerIndex && line.trim() === 'script: |',
);
assert.notEqual(
  scriptMarkerIndex,
  -1,
  'expected a "script: |" block scalar after the "Assign contributor" step; this suite extracts the workflow script from that block',
);

const blockLines = workflowLines.slice(scriptMarkerIndex + 1);
const firstContentLine = blockLines.find((line) => line.trim() !== '');
assert.notEqual(
  firstContentLine,
  undefined,
  'expected the "script: |" block of the "Assign contributor" step to be non-empty',
);
const blockIndent = firstContentLine.match(/^ */)[0];
assert.notEqual(
  blockIndent,
  '',
  'expected the "script: |" block content to be indented so the end of the block can be detected',
);

const scriptLines = [];
for (const line of blockLines) {
  if (line.trim() === '') {
    scriptLines.push('');
    continue;
  }
  if (!line.startsWith(blockIndent)) {
    break;
  }
  scriptLines.push(line.slice(blockIndent.length));
}

const AsyncFunction = Object.getPrototypeOf(async function () {}).constructor;
const runWorkflow = new AsyncFunction(
  'github',
  'context',
  'core',
  scriptLines.join('\n'),
);

const sliceBlock = (lines, startIndex, indent) => {
  const block = [];
  for (const line of lines.slice(startIndex)) {
    if (line.trim() !== '' && !line.startsWith(indent)) {
      break;
    }
    block.push(line);
  }
  return block;
};

const jobsIndex = workflowLines.findIndex((line) => line === 'jobs:');
assert.notEqual(
  jobsIndex,
  -1,
  'expected assign-issue.yml to declare a top-level "jobs:" mapping',
);
const jobsLines = sliceBlock(workflowLines, jobsIndex + 1, '  ');
const jobNames = jobsLines
  .map((line) => line.match(/^ {2}(\S+):$/))
  .filter((match) => match !== null)
  .map((match) => match[1]);

const assignJobIndex = jobsLines.findIndex(
  (line) => line === '  assign-issue:',
);
assert.notEqual(
  assignJobIndex,
  -1,
  'expected assign-issue.yml to declare an "assign-issue" job',
);
const assignJobLines = sliceBlock(jobsLines, assignJobIndex + 1, '    ');
const assignJob = assignJobLines.join('\n');

const conditionIndex = assignJobLines.findIndex(
  (line) => line.trim() === 'if: >-',
);
assert.notEqual(
  conditionIndex,
  -1,
  'expected the "assign-issue" job to be gated by an "if: >-" condition',
);
const assignJobCondition = sliceBlock(assignJobLines, conditionIndex + 1, '      ')
  .map((line) => line.trim())
  .join(' ')
  .replace(/\s+/g, ' ')
  .trim();

const ownerUserIdsMatch = assignJob.match(
  /^ {10}OWNER_USER_IDS: '(\[[\d,]*\])'$/m,
);
assert.notEqual(
  ownerUserIdsMatch,
  null,
  'expected the "Assign contributor" step to pass the docs/OWNER.md user IDs through an OWNER_USER_IDS environment variable',
);
const workflowOwnerUserIds = ownerUserIdsMatch[1];

const makeIssue = (assignees = [], state = 'open', number = 1) => ({
  number,
  state,
  assignees: assignees.map((login) => ({ login })),
  html_url: `https://github.com/WasmEdge/WasmEdge/issues/${number}`,
});

const OWNER_USER_IDS =
  '[10806,251849,274041,2776756,3313947,4726889,7088579,14789875,16274282,22004511,23314210,24819143,30090427,34829253,36074633,40065278,45785633,53310459,56215747,61797109,94267867]';

async function runScenario({
  body = '/assign @alice',
  payloadIssue = makeIssue(),
  issue = makeIssue(),
  issueError,
  commentError,
  conflictingIssues = [],
  assignable = true,
  eligibilityStatus = 404,
  assignmentResult = makeIssue(['Alice']),
  targetUserId = 4242,
  targetLookupError,
  ownerUserIds = OWNER_USER_IDS,
}) {
  const comments = [];
  const commentCalls = [];
  const addAssigneeCalls = [];
  const conflictScanParams = [];
  const targetLookups = [];
  const failures = [];
  const warnings = [];
  let issueFetches = 0;
  let eligibilityChecks = 0;
  const listForRepo = async () => {};
  const github = {
    rest: {
      issues: {
        createComment: async (params) => {
          commentCalls.push(structuredClone(params));
          if (commentError) {
            throw commentError;
          }
          comments.push(params.body);
          return { data: { id: 900 + comments.length } };
        },
        get: async () => {
          issueFetches += 1;
          if (issueError) {
            throw issueError;
          }
          return { data: structuredClone(issue) };
        },
        listForRepo,
        checkUserCanBeAssignedToIssue: async () => {
          eligibilityChecks += 1;
          if (!assignable) {
            const error = new Error('Not assignable');
            error.status = eligibilityStatus;
            throw error;
          }
        },
        addAssignees: async (params) => {
          addAssigneeCalls.push(structuredClone(params));
          return { data: structuredClone(assignmentResult) };
        },
      },
      users: {
        getByUsername: async (params) => {
          targetLookups.push(structuredClone(params));
          if (targetLookupError) {
            throw targetLookupError;
          }
          return { data: { login: params.username, id: targetUserId } };
        },
      },
    },
    paginate: async (endpoint, params) => {
      if (endpoint === listForRepo) {
        conflictScanParams.push(structuredClone(params));
        return structuredClone(conflictingIssues);
      }
      throw new Error('Unexpected paginated endpoint');
    },
  };
  const context = {
    repo: { owner: 'WasmEdge', repo: 'WasmEdge' },
    payload: {
      comment: {
        id: 12345,
        body,
        created_at: '2026-07-31T00:00:00Z',
        user: { login: 'hydai' },
      },
      issue: structuredClone(payloadIssue),
    },
  };
  const core = {
    setFailed: (message) => failures.push(message),
    warning: (message) => warnings.push(message),
  };
  const previousOwnerUserIds = process.env.OWNER_USER_IDS;
  process.env.OWNER_USER_IDS = ownerUserIds;
  let workflowError;
  try {
    await runWorkflow(github, context, core);
  } catch (error) {
    workflowError = error;
  } finally {
    if (previousOwnerUserIds === undefined) {
      delete process.env.OWNER_USER_IDS;
    } else {
      process.env.OWNER_USER_IDS = previousOwnerUserIds;
    }
  }
  return {
    added: addAssigneeCalls.length,
    addAssigneeCalls,
    commentCalls,
    comments,
    conflictScanParams,
    eligibilityChecks,
    failures,
    issueFetches,
    targetLookups,
    warnings,
    workflowError,
  };
}

test('declares the assign job as the only job in the workflow', () => {
  assert.deepEqual(jobNames, ['assign-issue']);
});

test('bounds runtime and blocks unauthorized egress', () => {
  assert.match(assignJob, /^ {4}timeout-minutes: 10$/m);
  assert.match(assignJob, /^ {10}egress-policy: block$/m);
  assert.match(
    assignJob,
    /^ {10}allowed-endpoints: >\n {12}api\.github\.com:443$/m,
  );
});

test('retries transient GitHub API failures', () => {
  assert.match(assignJob, /^ {10}retries: 3$/m);
});

test('gates the job on the maintainer allowlist and the /assign command', () => {
  assert.equal(
    assignJobCondition,
    'github.event.issue.pull_request == null && ' +
      "contains( fromJSON('[10806,251849,274041,2776756,3313947,4726889," +
      '7088579,14789875,16274282,22004511,23314210,24819143,30090427,' +
      "34829253,36074633,40065278,45785633,53310459,56215747,61797109,94267867]')," +
      ' github.event.comment.user.id ) && ' +
      "contains(github.event.comment.body, '/assign')",
    'this condition is the authorization boundary of the workflow; change it here only together with a deliberate review of the job gate',
  );
});

test('shares one owner allowlist between the job gate and the assign script', () => {
  assert.equal(
    workflowOwnerUserIds,
    OWNER_USER_IDS,
    'the OWNER_USER_IDS environment variable must list the docs/OWNER.md user IDs',
  );
  assert.ok(
    assignJobCondition.includes(`fromJSON('${OWNER_USER_IDS}')`),
    'a job-level "if" cannot read the env context, so the gate repeats the OWNER_USER_IDS list and both copies must stay identical',
  );
});

test('ignores a comment that only mentions /assign mid-text', async () => {
  const result = await runScenario({ body: 'use /assign @alice to claim' });

  assert.equal(result.workflowError, undefined);
  assert.equal(result.issueFetches, 0);
  assert.equal(result.added, 0);
  assert.deepEqual(result.comments, []);
});

test('ignores a comment starting with a word that extends /assign', async () => {
  const result = await runScenario({ body: '/assigned to me yesterday' });

  assert.equal(result.workflowError, undefined);
  assert.equal(result.issueFetches, 0);
  assert.equal(result.added, 0);
  assert.deepEqual(result.comments, []);
});

test('ignores a comment starting with /assignee', async () => {
  const result = await runScenario({ body: '/assignee is bob' });

  assert.equal(result.workflowError, undefined);
  assert.equal(result.issueFetches, 0);
  assert.equal(result.added, 0);
  assert.deepEqual(result.comments, []);
});

test('ignores a comment starting with a hyphenated extension of /assign', async () => {
  const result = await runScenario({ body: '/assign-issue.yml has a bug' });

  assert.equal(result.workflowError, undefined);
  assert.equal(result.issueFetches, 0);
  assert.equal(result.added, 0);
  assert.deepEqual(result.comments, []);
});

test('ignores an assign command that is not on the first line', async () => {
  const result = await runScenario({
    body: 'Thanks for volunteering!\n\n/assign @alice',
  });

  assert.equal(result.workflowError, undefined);
  assert.equal(result.issueFetches, 0);
  assert.equal(result.added, 0);
  assert.deepEqual(result.comments, []);
});

test('replies with usage guidance when the target is not separated', async () => {
  const result = await runScenario({ body: '/assign@alice' });

  assert.equal(result.workflowError, undefined);
  assert.equal(result.issueFetches, 0);
  assert.equal(result.added, 0);
  assert.deepEqual(result.comments, [
    '@hydai, use `/assign @username` with exactly one GitHub username.',
  ]);
});

test('replies with usage guidance for a multiline assign command', async () => {
  const result = await runScenario({ body: '/assign\n@alice' });

  assert.equal(result.workflowError, undefined);
  assert.equal(result.issueFetches, 0);
  assert.equal(result.added, 0);
  assert.deepEqual(result.comments, [
    '@hydai, use `/assign @username` with exactly one GitHub username.',
  ]);
});

test('replies with usage guidance for a malformed assign command', async () => {
  const result = await runScenario({ body: '/assign alice' });

  assert.equal(result.workflowError, undefined);
  assert.equal(result.issueFetches, 0);
  assert.equal(result.added, 0);
  assert.deepEqual(result.comments, [
    '@hydai, use `/assign @username` with exactly one GitHub username.',
  ]);
});

test('accepts an assign command with surrounding whitespace', async () => {
  const result = await runScenario({ body: '  /assign @alice \n' });

  assert.equal(result.workflowError, undefined);
  assert.equal(result.added, 1);
  assert.deepEqual(result.comments, ['Assigned this issue to @alice.']);
});

test('accepts an assign command followed by further comment text', async () => {
  const result = await runScenario({
    body: '/assign @alice\r\n\r\nThanks for picking this up!',
  });

  assert.equal(result.workflowError, undefined);
  assert.equal(result.added, 1);
  assert.deepEqual(result.comments, ['Assigned this issue to @alice.']);
});

test('accepts an assign command separated by a non-breaking space', async () => {
  const result = await runScenario({ body: '/assign\u00a0@alice' });

  assert.equal(result.workflowError, undefined);
  assert.equal(result.added, 1);
  assert.deepEqual(result.comments, ['Assigned this issue to @alice.']);
});

test('accepts a mixed-case assign command', async () => {
  const result = await runScenario({ body: '/Assign @alice' });

  assert.equal(result.workflowError, undefined);
  assert.equal(result.added, 1);
  assert.deepEqual(result.comments, ['Assigned this issue to @alice.']);
});

test('accepts an assign command containing zero-width characters', async () => {
  const result = await runScenario({ body: '\uFEFF/assign @alice\u200B' });

  assert.equal(result.workflowError, undefined);
  assert.equal(result.added, 1);
  assert.deepEqual(result.comments, ['Assigned this issue to @alice.']);
});

test('rejects assignment on a closed issue', async () => {
  const result = await runScenario({ issue: makeIssue([], 'closed') });

  assert.equal(result.workflowError, undefined);
  assert.equal(result.issueFetches, 1);
  assert.deepEqual(result.conflictScanParams, []);
  assert.equal(result.eligibilityChecks, 0);
  assert.equal(result.added, 0);
  assert.deepEqual(result.comments, [
    '@hydai, only open issues can be assigned.',
  ]);
});

test('reports when the target is already the sole assignee', async () => {
  const result = await runScenario({ issue: makeIssue(['Alice']) });

  assert.equal(result.workflowError, undefined);
  assert.equal(result.eligibilityChecks, 0);
  assert.equal(result.added, 0);
  assert.deepEqual(result.comments, [
    '@hydai, `@alice` is already assigned to this issue.',
  ]);
});

test('rejects an issue that is already assigned to someone else', async () => {
  const result = await runScenario({ issue: makeIssue(['Bob']) });

  assert.equal(result.workflowError, undefined);
  assert.equal(result.eligibilityChecks, 0);
  assert.equal(result.added, 0);
  assert.deepEqual(result.comments, [
    '@hydai, this issue is already assigned to `@Bob`. Each issue can be assigned to only one contributor.',
  ]);
});

test('omits the requested target from the blocking assignee list', async () => {
  const result = await runScenario({ issue: makeIssue(['Alice', 'Bob']) });

  assert.equal(result.workflowError, undefined);
  assert.equal(result.added, 0);
  assert.deepEqual(result.comments, [
    '@hydai, this issue is already assigned to `@Bob`. Each issue can be assigned to only one contributor.',
  ]);
});

test('assigns when a stale payload assignee has been removed', async () => {
  const result = await runScenario({ payloadIssue: makeIssue(['Bob']) });

  assert.equal(result.workflowError, undefined);
  assert.equal(result.issueFetches, 1);
  assert.equal(result.added, 1);
  assert.deepEqual(result.comments, ['Assigned this issue to @alice.']);
});

test('rejects a target who already holds another open issue', async () => {
  const result = await runScenario({
    conflictingIssues: [makeIssue(['Alice'], 'open', 2)],
  });

  assert.equal(result.workflowError, undefined);
  assert.equal(result.eligibilityChecks, 1);
  assert.deepEqual(result.targetLookups, [{ username: 'alice' }]);
  assert.equal(result.added, 0);
  assert.deepEqual(result.comments, [
    '@hydai, `@alice` is already assigned to [#2](https://github.com/WasmEdge/WasmEdge/issues/2). Contributors other than the maintainers, committers, and reviewers in docs/OWNER.md can be assigned to only one open issue at a time and cannot receive another until the current issue is resolved.',
  ]);
});

test('lets an owner target hold more than one open issue', async () => {
  const result = await runScenario({
    targetUserId: 2776756,
    conflictingIssues: [makeIssue(['Alice'], 'open', 2)],
  });

  assert.equal(result.workflowError, undefined);
  assert.equal(result.eligibilityChecks, 1);
  assert.deepEqual(result.targetLookups, [{ username: 'alice' }]);
  assert.deepEqual(result.conflictScanParams, []);
  assert.equal(result.added, 1);
  assert.deepEqual(result.comments, ['Assigned this issue to @alice.']);
});

test('keeps the one open issue limit for a target outside the owner allowlist', async () => {
  const result = await runScenario({
    targetUserId: 2776757,
    conflictingIssues: [makeIssue(['Alice'], 'open', 2)],
  });

  assert.equal(result.workflowError, undefined);
  assert.equal(result.conflictScanParams.length, 1);
  assert.equal(result.added, 0);
  assert.equal(result.comments.length, 1);
  assert.match(result.comments[0], /^@hydai, `@alice` is already assigned to /);
});

test('keeps the one open issue limit when the target account cannot be resolved', async () => {
  const targetLookupError = new Error('Not Found');
  targetLookupError.status = 404;
  const result = await runScenario({
    targetLookupError,
    conflictingIssues: [makeIssue(['Alice'], 'open', 2)],
  });

  assert.equal(result.workflowError, undefined);
  assert.equal(result.conflictScanParams.length, 1);
  assert.equal(result.added, 0);
  assert.equal(result.comments.length, 1);
  assert.match(result.comments[0], /^@hydai, `@alice` is already assigned to /);
});

test('reports an unexpected owner lookup failure', async () => {
  const targetLookupError = new Error('Server Error');
  targetLookupError.status = 500;
  const result = await runScenario({ targetLookupError });

  assert.equal(result.workflowError, undefined);
  assert.deepEqual(result.conflictScanParams, []);
  assert.equal(result.added, 0);
  assert.deepEqual(result.comments, [
    '@hydai, the `/assign` command failed unexpectedly. Please try again later.',
  ]);
  assert.deepEqual(result.failures, [
    'The /assign command failed: Server Error',
  ]);
});

test('reports a malformed owner allowlist without assigning', async () => {
  const result = await runScenario({ ownerUserIds: 'not-json' });

  assert.equal(result.workflowError, undefined);
  assert.deepEqual(result.targetLookups, []);
  assert.deepEqual(result.conflictScanParams, []);
  assert.equal(result.added, 0);
  assert.deepEqual(result.comments, [
    '@hydai, the `/assign` command failed unexpectedly. Please try again later.',
  ]);
  assert.equal(result.failures.length, 1);
  assert.match(result.failures[0], /^The \/assign command failed: /);
});

test('reports a target that cannot be assigned before scanning for conflicts', async () => {
  const result = await runScenario({ assignable: false });

  assert.equal(result.workflowError, undefined);
  assert.equal(result.eligibilityChecks, 1);
  assert.deepEqual(result.conflictScanParams, []);
  assert.equal(result.added, 0);
  assert.deepEqual(result.comments, [
    '@hydai, GitHub does not allow `@alice` to be assigned to this issue.',
  ]);
});

test('reports an unexpected eligibility-check failure', async () => {
  const result = await runScenario({
    assignable: false,
    eligibilityStatus: 403,
  });

  assert.equal(result.workflowError, undefined);
  assert.deepEqual(result.conflictScanParams, []);
  assert.equal(result.added, 0);
  assert.deepEqual(result.comments, [
    '@hydai, the `/assign` command failed unexpectedly. Please try again later.',
  ]);
  assert.deepEqual(result.failures, [
    'The /assign command failed: Not assignable',
  ]);
});

test('reports an unexpected issue-lookup failure', async () => {
  const issueError = new Error('Server Error');
  issueError.status = 500;
  const result = await runScenario({ issueError });

  assert.equal(result.workflowError, undefined);
  assert.equal(result.eligibilityChecks, 0);
  assert.equal(result.added, 0);
  assert.deepEqual(result.comments, [
    '@hydai, the `/assign` command failed unexpectedly. Please try again later.',
  ]);
  assert.deepEqual(result.failures, [
    'The /assign command failed: Server Error',
  ]);
});

test('reports when GitHub silently ignores the assignee', async () => {
  const result = await runScenario({ assignmentResult: makeIssue() });

  assert.equal(result.workflowError, undefined);
  assert.equal(result.added, 1);
  assert.deepEqual(result.comments, [
    '@hydai, GitHub did not allow `@alice` to be assigned to this issue.',
  ]);
});

test('reports when a concurrent assignment left another assignee in place', async () => {
  const result = await runScenario({
    assignmentResult: makeIssue(['Bob', 'Alice']),
  });

  assert.equal(result.workflowError, undefined);
  assert.equal(result.added, 1);
  assert.deepEqual(result.comments, [
    '@hydai, `@alice` was assigned alongside `@Bob`. Each issue can be assigned to only one contributor; please remove the extra assignees.',
  ]);
});

test('records a warning instead of failing when the reply cannot be posted', async () => {
  const commentError = new Error('You have exceeded a secondary rate limit');
  commentError.status = 403;
  const result = await runScenario({ commentError });

  assert.equal(result.workflowError, undefined);
  assert.equal(result.added, 1);
  assert.deepEqual(result.failures, []);
  assert.deepEqual(result.warnings, [
    'Failed to post the /assign reply: You have exceeded a secondary rate limit',
  ]);
});

test('assigns an eligible target and excludes pull requests and this issue from the conflict scan', async () => {
  const result = await runScenario({
    conflictingIssues: [
      { ...makeIssue(['Alice'], 'open', 7), pull_request: {} },
      makeIssue(['Alice'], 'open', 1),
    ],
  });

  assert.equal(result.workflowError, undefined);
  assert.equal(result.issueFetches, 1);
  assert.equal(result.eligibilityChecks, 1);
  assert.deepEqual(result.conflictScanParams, [
    {
      owner: 'WasmEdge',
      repo: 'WasmEdge',
      assignee: 'alice',
      state: 'open',
      per_page: 100,
    },
  ]);
  assert.deepEqual(result.addAssigneeCalls, [
    {
      owner: 'WasmEdge',
      repo: 'WasmEdge',
      issue_number: 1,
      assignees: ['alice'],
    },
  ]);
  assert.deepEqual(result.commentCalls, [
    {
      owner: 'WasmEdge',
      repo: 'WasmEdge',
      issue_number: 1,
      body: 'Assigned this issue to @alice.',
    },
  ]);
  assert.deepEqual(result.comments, ['Assigned this issue to @alice.']);
});
