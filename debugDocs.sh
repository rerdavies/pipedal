#!/bin/bash
# Run local jeckyl-hosted sevever for GitHub documentation page.
# See https://jekyllrb.com/docs/installation/ for instructions on how to isntall jeckyll locally.
# Then 
#    cd docs
#    bundle install
#
cd docs
bundle exec jekyll serve --host 0.0.0.0