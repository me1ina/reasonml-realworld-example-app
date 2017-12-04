open Models;

let show = ReasonReact.stringToElement;

let profile_image = {|http://i.imgur.com/Qr71crq.jpg|};
let second_image = {|"http://i.imgur.com/N4VcUeJ.jpg"|};

type action =
  | TagsFetched(array(string))
  | ShowMyFeed
  | ShowGlobalFeed
  | ArticlesFetched(articleList)
  | MyArticlesFetched(articleList)
  | TagArticlesFetched(articleList)
  | ShowTagList(string);

type state = {
  myFeedDisplay: ReactDOMRe.style,
  globalFeedDisplay: ReactDOMRe.style,
  tagFeedDisplay: ReactDOMRe.style,
  myFeedActiveClass: string,
  globalfeedActiveClass: string,
  tagFeedActiveClass: string,
  tags: array(string),
  articles: array(article),
  articleCount: int,
  showTagTab: bool,
  currentTagName: string
};

let initialState = () => {
  tags: [||],
  myFeedDisplay: ReactDOMRe.Style.make(~display="none", ()),
  globalFeedDisplay: ReactDOMRe.Style.make(~display="block", ()),
  tagFeedDisplay: ReactDOMRe.Style.make(~display="none", ()),
  myFeedActiveClass: "nav-link disabled",
  globalfeedActiveClass: "nav-link active",
  tagFeedActiveClass: "nav-link disabled",
  articles: [||],
  articleCount: 0,
  showTagTab: false,
  currentTagName: ""
};

let showTaggedArticles = (event) =>
  ShowTagList(ReactDOMRe.domElementToObj(ReactEventRe.Mouse.target(event))##innerText);

/* article page uses this decoder as well */
let decodeAuthor = (json) =>
  Json.Decode.{
    username: json |> field("username", string),
    bio: json |> optional(field("bio", string)),
    image: json |> optional(field("image", string)),
    following: json |> field("following", bool)
  };

let decodeArticles = (json) => {
  Json.Decode.{
    slug: json |> field("slug", string),
    title: json |> field("title", string),
    description: json |> field("description", string),
    body: json |> field("body", string),
    tagList: [||], /* json |> field("tagList", array(string)) */
    createdAt: json |> field("createdAt", string),
    updatedAt: json |> field("updatedAt", string),
    favorited: json |> field("favorited", bool),
    favoritesCount: json |> field("favoritesCount", int),
    author: json |> field("author", decodeAuthor)
  };
};

let populateTags = (reduce) => {
  let reduceTags = (_status, jsonPayload) => {

    jsonPayload |> Js.Promise.then_((result) => {
      let parsedPopularTags = Js.Json.parseExn(result);
      let tags = Json.Decode.(parsedPopularTags |> field("tags", array(string)));
      reduce((_) => TagsFetched(tags), ());

      tags |> Js.Promise.resolve
    }) |> ignore;
  };
  JsonRequests.getPoplarTags(reduceTags) |> ignore;
};

let reduceFeed = (reduceToAction, _state, jsonPayload) => {
  jsonPayload |> Js.Promise.then_((result) => {
    let parsedArticles = Js.Json.parseExn(result);

    let articleList = Json.Decode.{
      articles: parsedArticles |> field("articles", array(decodeArticles)),
      articlesCount: parsedArticles |> field("articlesCount", int)
    };

    reduceToAction(articleList);
    articleList |> Js.Promise.resolve
  })
};

let populateGlobalFeed = (reduce) => {
  let reduceFunc = (articleList) => reduce((_) => ArticlesFetched(articleList), ());
  /* Get the right page if there are more than 10 articles */
  JsonRequests.getGlobalArticles(reduceFeed(reduceFunc), Effects.getTokenFromStorage(), 10, 0) |> ignore;
};

let populateFeed = (reduce) => {
  let reduceFunc = (articleList) => reduce((_) => MyArticlesFetched(articleList), ());
  JsonRequests.getFeed(Effects.getTokenFromStorage(), reduceFeed(reduceFunc)) |> ignore
};

let showMyFeed = (event, {ReasonReact.state, reduce}) => {
  ReactEventRe.Mouse.preventDefault(event);
  populateFeed(reduce);
  reduce((_) => ShowMyFeed,());
};

let showGlobalFeed = (event, {ReasonReact.state, reduce}) => {
  ReactEventRe.Mouse.preventDefault(event);
  populateGlobalFeed(reduce);
  reduce((_) => ShowGlobalFeed,());
};

let goToArticle = (router, articleCallback, article, event, {ReasonReact.state}) => {
  ReactEventRe.Mouse.preventDefault(event);
  articleCallback(article);
  DirectorRe.setRoute(router,"/article")
};

let renderTag = ({ReasonReact.state, reduce}, index, tag) => {
  <a onClick=(reduce(showTaggedArticles)) href="#" key=(string_of_int(index)) className="tag-pill tag-default"> (show(tag)) </a>
};

let renderArticle = (handle, router, articleCallback, index, article) =>
  <div key=(string_of_int(index)) className="article-preview">
    <div>
      <div className="article-meta">
        <a href="profile.html" />
        <div className="info">
          <a href="" className="author"> (show(article.author.username)) </a>
          <span className="date"> (show(Js.Date.fromString(article.createdAt) |> Js.Date.toDateString)) </span>
        </div>
        <button className="btn btn-outline-primary btn-sm pull-xs-right">
          <i className="ion-heart" />
          (show("0"))
        </button>
      </div>
      <a href="#" onClick=(handle(goToArticle(router, articleCallback, article))) className="preview-link">
        <h1>
          (show(article.title))
        </h1>
        <p> (show(article.description)) </p>
        <span> (show("Read more...")) </span>
      </a>
    </div>
  </div>;

let component = ReasonReact.reducerComponent("Home");

let make = (~articleCallback, ~router, _children) => {
  ...component,
  initialState: initialState,
  reducer: (action, state) =>
    switch action {
    | TagsFetched(tagList) => ReasonReact.Update({...state, tags: tagList})
    | ShowMyFeed => ReasonReact.Update({
      ...state,
      myFeedDisplay: ReactDOMRe.Style.make(~display="block", ()),
      globalFeedDisplay: ReactDOMRe.Style.make(~display="none", ()),
      tagFeedDisplay: ReactDOMRe.Style.make(~display="none", ()),
      myFeedActiveClass: "nav-link active",
      globalfeedActiveClass: "nav-link disabled",
      tagFeedActiveClass: "nav-link disabled"
    })
    | ShowGlobalFeed => ReasonReact.Update({
      ...state,
      myFeedDisplay: ReactDOMRe.Style.make(~display="none", ()),
      globalFeedDisplay: ReactDOMRe.Style.make(~display="block", ()),
      tagFeedDisplay: ReactDOMRe.Style.make(~display="none", ()),
      myFeedActiveClass: "nav-link disabled",
      globalfeedActiveClass: "nav-link active",
      tagFeedActiveClass: "nav-link disabled"
    })
    | ArticlesFetched(articleList) => ReasonReact.Update({
      ...state,
      articles: articleList.articles,
      articleCount: articleList.articlesCount,
      tagFeedDisplay: ReactDOMRe.Style.make(~display="none", ())
    })
    | MyArticlesFetched(articleList) => ReasonReact.Update({
      ...state,
      articles: articleList.articles,
      articleCount: articleList.articlesCount
    })
    | TagArticlesFetched(articleList) => ReasonReact.Update({
      ...state,
      articles: articleList.articles,
      myFeedDisplay: ReactDOMRe.Style.make(~display="none", ()),
      globalFeedDisplay: ReactDOMRe.Style.make(~display="none", ()),
      tagFeedDisplay: ReactDOMRe.Style.make(~display="block", ())
    })
    | ShowTagList(currentTagName) => ReasonReact.UpdateWithSideEffects({
      ...state,
      currentTagName: currentTagName,
      myFeedDisplay: ReactDOMRe.Style.make(~display="none", ()),
      globalFeedDisplay: ReactDOMRe.Style.make(~display="none", ()),
      tagFeedDisplay: ReactDOMRe.Style.make(~display="block", ()),
      myFeedActiveClass: "nav-link disabled",
      globalfeedActiveClass: "nav-link disabled",
      tagFeedActiveClass: "nav-link active"
    }, (self) => {
      let reduceFunc = (articleList) => self.reduce((_) => TagArticlesFetched(articleList), ());
      JsonRequests.getArticlesByTag(reduceFeed(reduceFunc), currentTagName, Effects.getTokenFromStorage()) |> ignore;
    })
    },
  didMount: (self) => {
    populateTags(self.reduce);
    populateGlobalFeed(self.reduce);
    ReasonReact.NoUpdate
  },
  render: (self) => {
    let {ReasonReact.state} = self;
    let currentTagName = state.currentTagName;
    <div className="home-page">
      <div className="banner">
        <div className="container">
          <h1 className="logo-font"> (show("conduit")) </h1>
          <p> (show("A place to share your knowledge.")) </p>
        </div>
      </div>
      <div className="container page">
        <div className="row">
          <div className="col-md-9">
            <div className="feed-toggle">
              <ul className="nav nav-pills outline-active">
                <li className="nav-item">
                  <a className=(state.myFeedActiveClass) href="#" onClick=(self.handle(showMyFeed))> (show("Your Feed")) </a>
                </li>
                <li className="nav-item">
                  <a className=(state.globalfeedActiveClass) href="#" onClick=(self.handle(showGlobalFeed))> (show("Global Feed")) </a>
                </li>
                <li className="nav-item" style=(state.tagFeedDisplay)>
                  <a  className=(state.tagFeedActiveClass) href="#"> (show({j|#$currentTagName|j} )) </a>
                </li>
              </ul>
            </div>
            <div className="article-preview" style=(state.myFeedDisplay)>
              {Array.mapi(renderArticle(self.handle, router, articleCallback), state.articles) |> ReasonReact.arrayToElement}
            </div>
            <div style=(state.globalFeedDisplay)>
              {Array.mapi(renderArticle(self.handle, router, articleCallback), state.articles) |> ReasonReact.arrayToElement}
            </div>
            <div style=(state.tagFeedDisplay)>
              {Array.mapi(renderArticle(self.handle, router, articleCallback), state.articles) |> ReasonReact.arrayToElement}
            </div>
          </div>
          <div className="col-md-3">
            <div className="sidebar">
              <p> (show("Popular Tags")) </p>
              <div className="tag-list" >
                (Array.mapi(renderTag(self),state.tags) |> ReasonReact.arrayToElement)
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>
  }
};
