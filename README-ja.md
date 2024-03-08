<div align="right">

  [Readme in English](README.md) | [中文](README-zh.md) | [正體中文](README-zh-TW.md)

</div>

<div align="center">

![WasmEdge Logo](/docs/wasmedge-runtime-logo.png)

# [🤩 WasmEdge は、ご自身のデバイスで LLM を実行する最も簡単で早い方法です。 🤩](https://www.secondstate.io/articles/wasm-runtime-agi/)

WasmEdge は軽量、高性能、拡張可能な WebAssembly ランタイムです。現在、[最速の Wasm VM](https://ieeexplore.ieee.org/document/9214403) になります。WasmEdge は、[CNCF](https://www.cncf.io/) が主催する公式サンドボックスプロジェクトでです。その[ユースケース](https://wasmedge.org/book/en/use_cases.html)には、モダンなウェブアプリケーションアーキテクチャ(Isomorphic & Jamstack　アプリケーション)、エッジクラウド上のマイクロサービス、サーバーレス　SaaS API、組み込み機能、スマートコントラクト、スマートデバイスなどが含まれます。

![build](https://github.com/WasmEdge/WasmEdge/workflows/build/badge.svg)
[![codecov](https://codecov.io/gh/WasmEdge/WasmEdge/branch/master/graph/badge.svg)](https://codecov.io/gh/WasmEdge/WasmEdge)
[![CodeQL](https://github.com/WasmEdge/WasmEdge/actions/workflows/codeql-analysis.yml/badge.svg)](https://github.com/WasmEdge/WasmEdge/actions/workflows/codeql-analysis.yml)
[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2FWasmEdge%2FWasmEdge.svg?type=shield)](https://app.fossa.com/projects/git%2Bgithub.com%2FWasmEdge%2FWasmEdge?ref=badge_shield)
[![CII Best Practices](https://bestpractices.coreinfrastructure.org/projects/5059/badge)](https://bestpractices.coreinfrastructure.org/projects/5059)

</div>

# クイックスタートガイド

🚀 WasmEdge の[インストール](https://wasmedge.org/docs/start/install) \
🤖 WasmEdge への[ビルド](https://wasmedge.org/docs/category/build-wasmedge-from-source)と[コントリビュート](https://wasmedge.org/docs/contribute/) \
⌨️ CLIまたは[Docker](https://wasmedge.org/docs/start/getting-started/quick_start_docker)からスタンドアロンのWasmプログラムまたは[JavaScriptプログラム](https://wasmedge.org/docs/category/develop-wasm-apps-in-javascript)を[実行](https://wasmedge.org/docs/category/running-with-wasmedge) \
🔌 [Go](https://wasmedge.org/docs/category/go-sdk-for-embedding-wasmedge)、[Rust](https://wasmedge.org/docs/category/rust-sdk-for-embedding-wasmedge)、[C](https://wasmedge.org/docs/category/c-sdk-for-embedding-wasmedge)アプリにWasm関数を組み込みます \
🛠 [Kubernetes](https://wasmedge.org/docs/category/deploy-wasmedge-apps-in-kubernetes)、[データストリーミングフレームワーク](https://wasmedge.org/docs/embed/use-case/yomo)、[ブロックチェーン](https://medium.com/ethereum-on-steroids/running-ethereum-smart-contracts-in-a-substrate-blockchain-56fbc27fc95a)を使用してWasmランタイムを管理し、オーケストレーションする \
📚 **[公式ドキュメントをチェック](https://wasmedge.org/docs/)**

# イントロ

WasmEdge ランタイムは、含まれる WebAssembly バイトコードプログラムに対して、明確に定義された実行サンドボックスを提供します。ランタイムは、オペレーティングシステムリソース（ファイルシステム、ソケット、環境変数、プロセスなど）とメモリ空間の分離と保護を提供します。WasmEdge の最も重要なユースケースは、ソフトウェア製品（SaaS、Software-Defined Vehicle、エッジノード、あるいはブロックチェーンノードなど）のプラグインとして、ユーザー定義コードやコミュニティ貢献コードを安全に実行することになります。これにより、サードパーティの開発者、ベンダー、サプライヤー、コミュニティメンバーがソフトウェア製品を拡張し、カスタマイズすることが可能になります。**[詳細はこちら](https://wasmedge.org/docs/contribute/users)**

## パフォーマンス

* [A Lightweight Design for Highperformance Serverless Computing](https://arxiv.org/abs/2010.07115)、IEEE Software に掲載、Jan 2021。[https://arxiv.org/abs/2010.07115](https://arxiv.org/abs/2010.07115)
* [Performance Analysis for Arm vs. x86 CPUs in the Cloud](https://www.infoq.com/articles/arm-vs-x86-cloud-performance/)、infoQ.com に掲載、2021年1月。[https://www.infoq.com/articles/arm-vs-x86-cloud-performance/](https://www.infoq.com/articles/arm-vs-x86-cloud-performance/)
* [WasmEdge is the fastest WebAssembly Runtime in Suborbital Reactr test suite](https://blog.suborbital.dev/suborbital-wasmedge)、2021年12月

## 機能

WasmEdge は、C/C++、Rust、Swift、AssemblyScript、または Kotlin のソースコードからコンパイルされた標準的な WebAssembly バイトコードプログラムを実行できます。サードパーティの ES6、CJS、NPM モジュールを含む [JavaScript](https://wasmedge.org/docs/category/develop-wasm-apps-in-javascript) を、安全、高速、軽量、ポータブル、コンテナ化されたサンドボックスで実行します。また、これらの言語の混合（例えば、[JavaScript API を実装するために Rust を使用する](https://wasmedge.org/docs/develop/javascript/rust)）、[Fetch](https://wasmedge.org/docs/develop/javascript/networking#fetch-client)API、エッジサーバー上の[サーバーサイドレンダリング(SSR)](https://wasmedge.org/docs/develop/javascript/ssr)機能もサポートしています。

WasmEdge は、[すべての標準的な WebAssembly 機能と多くの提案されている拡張機能](https://wasmedge.org/docs/start/wasmedge/extensions/proposals)をサポートしています。また、クラウドネイティブやエッジコンピューティングの用途に合わせた拡張機能も多数サポートしています（[WasmEdge ネットワークソケット](https://wasmedge.org/docs/category/socket-networking)、[Postgres および MySQL ベースのデータベースドライバ](https://wasmedge.org/docs/category/database-drivers)、[WasmEdge AI 拡張機能](https://wasmedge.org/docs/category/ai-inference)など）。

 **WasmEdge の[テクニカルハイライト](https://wasmedge.org/docs/start/wasmedge/features)についてはこちらをご覧ください。**

## インテグレーションと管理

WasmEdge とそれに含まれる wasm プログラムは、新規プロセスとして [CLI](https://wasmedge.org/docs/category/running-with-wasmedge) から起動することも、既存プロセスから起動することもできます。既存のプロセス（例えば、実行中の [Go](https://wasmedge.org/docs/category/go-sdk-for-embedding-wasmedge) や [Rust](https://wasmedge.org/docs/category/rust-sdk-for-embedding-wasmedge) プログラムから起動した場合、WasmEdge は単に関数としてプロセス内で実行されます。現在のところ、WasmEdge はまだスレッドセーフではありません。独自のアプリケーションやクラウドネイティブフレームワークで WasmEdge を使用するには、以下のガイドを参照してください。

* [WasmEdge をホストアプリケーションに組み込む](https://wasmedge.org/docs/embed/overview)
* [コンテナツールを使用した WasmEdge インスタンスのオーケストレーションと管理](https://wasmedge.org/docs/category/deploy-wasmedge-apps-in-kubernetes)
* [WasmEdge アプリを Dapr マイクロサービスとして実行する](https://wasmedge.org/docs/develop/rust/dapr)

# コミュニティ

## コントリビュート

WasmEdge プロジェクトにコントリビュートしたい場合は、[CONTRIBUTING](https://wasmedge.org/docs/contribute/overview) ドキュメントを参照してください。アイデアをお探しなら、["help wanted" issues](https://github.com/WasmEdge/WasmEdge/issues?q=is%3Aissue+is%3Aopen+label%3A%22help+wanted%22)をチェックしてください！

## ロードマップ

[プロジェクトロードマップ](https://github.com/WasmEdge/WasmEdge/blob/master/docs/ROADMAP.md)で、WasmEdge の今後の機能と計画をご確認ください。

## コンタクト

ご質問がある場合は、関連プロジェクトの GitHub issue を開くか、以下のチャンネルにご参加ください:

* メーリングリスト: [WasmEdge@googlegroups.com](https://groups.google.com/g/wasmedge/) にメールを送信
* Discord: [WasmEdge Discord サーバー](https://discord.gg/h4KDyB8XTt)に参加してください！
* Slack: [CNCF Slack](https://slack.cncf.io/) の #WasmEdge チャンネルに参加する。
* Twitter: [Twitter](https://twitter.com/realwasmedge) で @realwasmedge をフォローする。

## 採用者

プロジェクトで WasmEdge を使用している[採用者リスト](https://wasmedge.org/docs/contribute/users/)をご覧ください。

## コミュニティミーティング

月に一度、コミュニティミーティングを開催し、新機能の紹介や新しいユースケースのデモ、質疑応答などを行います。どなたでもご参加いただけます！

時間: 毎月第1火曜日午後11時（香港時間）／午前7時（太平洋標準時）。

[パブリックミーティングのアジェンダ／ノート](https://docs.google.com/document/d/1iFlVl7R97Lze4RDykzElJGDjjWYDlkI8Rhf8g4dQ5Rk/edit#) | [Zoom リンク](https://us06web.zoom.us/j/89156807241?pwd=VHl5VW5BbmY2eUtTYkY0Zm9yUHRRdz09)

# ライセンス

[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2FWasmEdge%2FWasmEdge.svg?type=large)](https://app.fossa.com/projects/git%2Bgithub.com%2FWasmEdge%2FWasmEdge?ref=badge_large)
