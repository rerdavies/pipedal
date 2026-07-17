---
layout: news
title: News
---

# News


<div style="margin-left: 36px; padding-bottom: 24px;">
    <p style="margin-bottom: 0px; font-size: 14px;color: #888;">June 2, 2026</p>
    <h4 style="margin-top: 0px;"><a href='https://rerdavies.github.io/PiPedal20'>Announcing PiPedal 2.0!</a></h4>
</div>

https://rerdavies.github.io/PiPedal20

{% for post in site.posts %}
<div style="margin-left: 36px; padding-bottom: 24px;">
    <p style="margin-bottom: 0px; font-size: 14px;color: #888;">{{post.date| date: "%B %d, %Y"}}</p>
    <h4 style="margin-top: 0px;"><a href='/pipedal/{{ post.url }}'> {{ post.title }}</a></h4>
</div>
{% endfor %}
