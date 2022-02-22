# Contributing

Welcome to x64dbg! This document is relevant for you if you want to contribute to the project. If you just want to use x64dbg, go to the [website](https://x64dbg.com) instead.

## Overview

This is a list of things you can do to help us out (in no particular order). Each item will be expanded upon later in the document.

- [Compile x64dbg](https://github.com/x64dbg/x64dbg/wiki/Compiling-the-whole-project) and add new features ([easy issues](http://easy.x64dbg.com) are a good place to start).
- [Report bugs](http://report.x64dbg.com) at the issue tracker.
- Add feature requests to the [issue tracker](http://issues.x64dbg.com).
- [Write a blogpost](https://x64dbg.com/blog/2016/07/09/Looking-for-writers.html) for the [official blog](http://blog.x64dbg.com).
- [Contact us](https://x64dbg.com/#contact) and talk about x64dbg.
- Send a [donation](http://donate.x64dbg.com) to support the project.
- [Translate](http://translate.x64dbg.com) x64dbg (contact us if your language isn't listed).
- Help us improve the [documentation](https://github.com/x64dbg/docs/issues).

### Compile x64dbg

There is a guide to [compiling the whole project](https://github.com/x64dbg/x64dbg/wiki/Compiling-the-whole-project) available. This might seem difficult at first, but if you install the correct versions of the *Prerequisites* it will be a breeze.

Compiling x64dbg is very useful to us (even if you don't add any code). Your experience can improve this guide and help new contributors.

#### Getting started with development

As with any open source project, documentation is lacking and the code can seem very daunting at first. Here is a list of resources that can help you understand the architecture and get you started.

- [Architecture of x64dbg](https://x64dbg.com/blog/2016/10/04/architecture-of-x64dbg.html) (**must read**)
- [The x64dbg threading model](https://x64dbg.com/blog/2016/10/20/threading-model.html) (**must read**)
- [User interface design principles](https://x64dbg.com/blog/2016/08/08/user-interface-design-principles.html) blog post. It explains some of the design philosophy.
- [Control flow graph](https://x64dbg.com/blog/2016/07/27/Control-flow-graph.html) blog post. The post links to the relevant code sections.
- Blog post about the [plugin SDK](https://x64dbg.com/blog/2016/07/30/x64dbg-plugin-sdk.html). Writing an x64dbg plugin can also help you understand the code structure.

This is by no means an exhaustive list and we are still working on lowering the barrier for new contributors. The feedback of new contributors is vital to reaching this goal.

#### Sending a pull request

Here is a little guide on how to do a clean pull request for people who don't yet know how to use git. We recommend using [Git Extensions](https://gitextensions.github.io), but any git interface is fine.

1. First we need to [fork](https://help.github.com/articles/fork-a-repo/) the actual x64dbg repo on our github account.
2. When the fork is finished, clone the repo (`git clone https://github.com/myname/x64dbg.git`).
3. When pushing new features/bug/whatever to a github project the best practice is to create branches. The command `git checkout -b my-branch-name` will automatically create a branch and check it out.
4. Make all the changes you want and when finishing it, use `git add myfiles` to add it to the repo.
5. Commit your change. `git commit -m 'a message about what you changed'`. The change are applied to your local git repo.
6. Push it to your `origin`. The `origin` is your repo which is hosted on github. `git push --set-upstream origin your-branch-name`.
7. Sync with the `upstream` repo, the real x64dbg repo. `git remote add upstream https://github.com/x64dbg/x64dbg.git`, using `git remote -v` will show which origin/upstream are setup in the local repo.
8. Sync your fork with the `upstream`, `git fetch upstream`. Now checkout your local `development` branch again `git checkout development` and merge the upstream `git merge upstream/development`.
9. Time to create the pull request! Using the github ui, go to your account/repo, select the branch you already pushed, and click `Pull request`. Review your pull request and send it.

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

There are several ways to reach out to the community of x64dbg developers, contributors and users. Chat channels consist of a [Telegram](https://telegram.me/x64dbg), [Gitter](http://gitter.x64dbg.com/) and [IRC](https://webchat.freenode.net/?channels=x64dbg) channel. Most questions regarding contributing, developing and using x64dbg can be answered here. To ensure channel cohesion a bot will sync messages across all three channels. (when it is not down ;))

### Translate

To help translate x64dbg, just head over to http://translate.x64dbg.com, click a language you want to translate and start filling in entries.

### Improve the documentation

If you see any room for improvement in the [documentation](http://help.x64dbg.com), just send a pull request or contact us to discuss your changes.

### Triage Issues [![Open Source Helpers](https://www.codetriage.com/x64dbg/x64dbg/badges/users.svg)](https://www.codetriage.com/x64dbg/x64dbg)

You can triage issues which may include reproducing bug reports or asking for vital information, such as version numbers or reproduction instructions. If you would like to start triaging issues, one easy way to get started is to [subscribe to x64dbg on CodeTriage](https://www.codetriage.com/x64dbg/x64dbg).
