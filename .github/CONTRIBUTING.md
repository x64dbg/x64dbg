# Contributing

Welcome to x64dbg! This document is relevant for you if you want to contribute to the project. If you just want to use x64dbg, go to the [website](https://x64dbg.com) instead.

## Overview

This is a list of things you can do to help us out (in no particular order). Each item will be expanded upon later in the document.

- [Compile x64dbg](https://github.com/x64dbg/x64dbg/wiki/Compiling-the-whole-project) and add new features ([good first issues](https://easy.x64dbg.com) are a good place to start).
- Add report bugs of feature requests to the [issue tracker](https://issues.x64dbg.com).
- [Write a blogpost](https://x64dbg.com/blog/2016/07/09/Looking-for-writers.html) for the [official blog](https://blog.x64dbg.com).
- [Contact us](https://x64dbg.com/#contact) and chat about x64dbg.
- Send a [donation](https://donate.x64dbg.com) to support the project.
- [Translate](https://translate.x64dbg.com) x64dbg (reach out if your language is not listed).
- Help us improve the [documentation](https://help.x64dbg.com).

### Compile x64dbg

There is a guide to [compiling the whole project](https://github.com/x64dbg/x64dbg/wiki/Compiling-the-whole-project) available. We recently spent a lot of effort on making this as seamless as possible, but if you have any difficulties, reach out on Discord (`#development` channel)!

[![](https://dcbadge.limes.pink/api/server/PRfRYbt)](https://discord.x64dbg.com)

#### Getting started with development

As with any open source project, documentation is lacking and the code can seem very daunting at first. Here is a list of resources that can help you understand the architecture and get you started.

- [Architecture of x64dbg](https://x64dbg.com/blog/2016/10/04/architecture-of-x64dbg.html) (**must read**)
- [The x64dbg threading model](https://x64dbg.com/blog/2016/10/20/threading-model.html) (**must read**)
- [User interface design principles](https://x64dbg.com/blog/2016/08/08/user-interface-design-principles.html) blog post. It explains some of the design philosophy.
- [Control flow graph](https://x64dbg.com/blog/2016/07/27/Control-flow-graph.html) blog post. The post links to the relevant code sections.
- Blog post about the [plugin SDK](https://x64dbg.com/blog/2016/07/30/x64dbg-plugin-sdk.html). Writing an x64dbg plugin can also help you understand the code structure.
- Check out the [DeepWiki page](https://deepwiki.com/x64dbg/x64dbg) and ask Devin where to start.

This is by no means an exhaustive list and we are still working on lowering the barrier for new contributors. The feedback of new contributors is vital to reaching this goal.

#### Sending a pull request

Here is a little guide on how to do a clean pull request for people who don't yet know how to use git:

1. First we need to [fork](https://github.com/x64dbg/x64dbg/fork) the upstream x64dbg repo on our GitHub account.
2. When the fork is finished, clone the repo:
   ```sh
   git clone --recursive https://github.com/<yourusername>/x64dbg.git
   ```
3. Create a [feature branch](https://docs.github.com/en/pull-requests/collaborating-with-pull-requests/proposing-changes-to-your-work-with-pull-requests/about-branches) to isolate your changes:
   ```sh
   git checkout -B my-feature
   ```
4. Make all the changes you want and when finished, add the files to the [staging area](https://git-scm.com/about/staging-area):
   ```sh
   git add .
   ```
5. Create a [commit](https://docs.github.com/en/pull-requests/committing-changes-to-your-project/creating-and-editing-commits/about-commits) in your local git repository from the changes in your staging area:
   ```sh
   git commit -m "Added a cool feature"
   ```
6. Push the changes to your fork:
   ```sh
   git push --set-upstream origin my-feature
   ```
7. Time to create the pull request! Navigate to your forked repository in the browser and click the green `Pull request` button.

For a more in-depth tutorial, you can look at GitHub's official [contributing to a project](https://docs.github.com/en/get-started/exploring-projects-on-github/contributing-to-a-project) guide.

Happy PRs!

### Report bugs

If you want to have the highest chance of getting your problem solved, you are going to have to put in some effort. The vital things are:

1. Search the issue tracker to see if your bug has not been reported already.
2. Give concrete steps on how to reproduce your bug.
3. Tell us exactly which version of x64dbg you used and the environment(s) you reproduced the bug in.

You can take a look at the [issue template](https://github.com/x64dbg/x64dbg/blob/development/.github/ISSUE_TEMPLATE.md) for more details.

### Request features

Feature requests are often closed because they are out of scope. If you request one anyway, make sure to give a clear description of the desired behaviour and give clear examples of cases where your feature would be useful.

We understand that it can be disappointing to not get your feature implemented, but opening an issue is the best way to communicate it regardless.

### Write a blogpost

The x64dbg blog is open to all contributors (foreign and domestic). We encourage anyone who has an interesting encounter with the x64dbg code base, or a use case to share it with the community. For a guideline on how/what to contribute see the [blog post](https://x64dbg.com/blog/2016/07/09/Looking-for-writers.html) about contributing to the blog. Don't worry about contributing complex posts, we welcome ALL experience levels to add content to the blog!

### Contact us

There are several ways to reach out to the community of x64dbg developers, contributors and users:

[![Discord](https://img.shields.io/badge/chat-on%20Discord-green.svg)](https://discord.x64dbg.com) [![Slack](https://img.shields.io/badge/chat-on%20Slack-red.svg)](https://slack.x64dbg.com) [![Gitter](https://img.shields.io/badge/chat-on%20Gitter-lightseagreen.svg)](https://gitter.im/x64dbg/x64dbg) [![Matrix](https://img.shields.io/badge/chat-on%20Matrix-yellowgreen.svg)](https://riot.im/app/#/room/#x64dbg:matrix.org) [![IRC](https://img.shields.io/badge/chat-on%20IRC-purple.svg)](https://web.libera.chat/#x64dbg)

Discord is the main communication channel, but the other platforms are synchronized with the `#general` channel.

### Translate

To help translate x64dbg, just head over to https://translate.x64dbg.com, click a language you want to translate and start filling in entries.

### Improve the documentation

If you see any room for improvement in the [documentation](https://help.x64dbg.com), just send a pull request or contact us to discuss your changes.

### Triage Issues [![Open Source Helpers](https://www.codetriage.com/x64dbg/x64dbg/badges/users.svg)](https://www.codetriage.com/x64dbg/x64dbg)

You can triage issues which may include reproducing bug reports or asking for vital information, such as version numbers or reproduction instructions. If you would like to start triaging issues, one easy way to get started is to [subscribe to x64dbg on CodeTriage](https://www.codetriage.com/x64dbg/x64dbg).
