---
layout: news
title: News
---

# News

{% for post in site.posts %}
<div style="margin-left: 36px; padding-bottom: 24px;">
    <p style="margin-bottom: 0px; font-size: 14px;color: #888;">{{post.date| date: "%B %d, %Y"}}</p>
    <h4 style="margin-top: 0px;"><a href='/pipedal/{{ post.url }}'> {{ post.title }}</a></h4>
</div>
{% endfor %}
