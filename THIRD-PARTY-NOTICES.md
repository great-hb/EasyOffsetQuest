# Source attribution

EasyOffsetQuest is an independent, reduced Quest port. It is not an official EasyOffset release.

- **EasyOffset**, Reezonate, copyright 2021–2026. The v2 preset schema, transform semantics, default values, and panel layout follow the supplied EasyOffset reference. Its MIT license is included as `LICENSE-EasyOffset`.
- **CongXinJian**, lxtend / lixiangwuxian. The supplied source provides the starting Quest lifecycle, gesture, automatic pivot tracker, scene hooks, and line gizmos. Profile storage, UI, controller update, and offset math were replaced or adapted. The supplied tree has no license file; this notice records attribution and does not relicense those original portions.
- Runtime dependencies: beatsaber-hook 6.4.2, Quest BSML 0.4.55, custom-types 0.18.4, paper2_scotland2 4.7.0, tinyxml2 10.0.0, and Scotland2. Original licenses remain in the restored dependency trees. Dependencies are installed as separate QMODs.

References: [EasyOffset](https://github.com/Reezonate/EasyOffset), [CongXinJian](https://github.com/lixiangwuxian/CongXinJian).

The local tinyxml2 dependency QMOD changes only its package-version declaration to 1.40.8. The library itself is unmodified; the upstream archive and checksums are preserved separately.
