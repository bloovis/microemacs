# MicroEMACS User's Guide

This is the web version of the MicroEMACS User's Guide.
It uses Hugo to generate a static HTML web site for the guide.

To generate the guide, first install [Hugo](https://gohugo.io/).

The guide uses the [hugo-book theme](https://github.com/alex-shpak/hugo-book)
as a git submodule.  To load the theme, do this:

    git submodule init
    git submodule update

Then you can build the guide using:

    hugo

To view the guide on your local machine, use this:

    hugo server

Then point your browser to the URL printed by the above command,
typically something like <http://localhost:1313/meguide/> .

To upload the guide to your web site, use something like this:

    rsync -av "$@" public/ mylogin@www.example.com:/var/www/html/meguide

This assumes the guide will be at this URL: `https://www.example.com/meguide/` .
Be sure to set the `baseURL` definition in `hugo.toml` to the actual
URL of your site.
