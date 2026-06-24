// Copyright 2025 Neovex Authors. All rights reserved.
// Study Mode Navigation Throttle - blocks distracting websites
// YouTube is explicitly EXCLUDED from blocking.

#include "chrome/browser/study_mode/study_mode_navigation_throttle.h"

#include <algorithm>
#include <set>
#include <string>
#include <vector>

#include "base/logging.h"
#include "base/no_destructor.h"
#include "base/strings/string_util.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/navigation_throttle_registry.h"
#include "services/network/public/cpp/resource_request.h"
#include "net/url_request/redirect_info.h"
#include "url/gurl.h"

namespace {

bool g_study_mode_enabled = false;

// User-configurable whitelist (always includes youtube.com)
std::vector<std::string>& GetUserWhitelist() {
  static base::NoDestructor<std::vector<std::string>> whitelist;
  return *whitelist;
}

// The hardcoded list of distracting domains (youtube.com excluded).
const char* const kBlockedDomains[] = {
    "0xdd.org.ru",
    "4chan.org",
    "8kun.top",
    "9to5google.com",
    "9to5mac.com",
    "9to5toys.com",
    "aap.com.au",
    "abc.go.com",
    "abcnews.go.com",
    "abqjournal.com",
    "adelaidenow.com.au",
    "adultswim.com",
    "afp.com",
    "afr.com",
    "agar.io",
    "ahchealthenews.com",
    "aidungeon.io",
    "albumoftheyear.org",
    "aljazeera.com",
    "allmusic.com",
    "alternet.org",
    "amazon.co.uk",
    "amazon.com",
    "americanthinker.com",
    "anandtech.com",
    "androidauthority",
    "androidpit.com",
    "animalplanet.com",
    "apnews.com",
    "apple.com",
    "arstechnica.com",
    "asofterworld.com",
    "astralcodexten.com",
    "audiodiscourse.com",
    "avclub.com",
    "axios.com",
    "bandcamp.com",
    "baomi.tv",
    "baraag.net",
    "baraza.africa",
    "barrons.com",
    "bbc.co.uk",
    "bbc.com",
    "bbs.9tail.net",
    "beehaw.org",
    "beekeeping.ninja",
    "beforeitsnews.com",
    "behance.net",
    "bendigoadvertiser.com.au",
    "bgr.com",
    "bib.actionsack.com",
    "bibliogram.art",
    "bibliogram.ethibox.fr",
    "bibliogram.hamster.dance",
    "bibliogram.nixnet.services",
    "bibliogram.pussthecat.org",
    "bibliogram.synopta.org",
    "biomassmagazine.com",
    "bipartisanreport.com",
    "bird.nogafam.es",
    "bird.trom.tf",
    "birdsite.xanny.family",
    "bitmoji.com",
    "bleacherreport.com",
    "blockchain.info",
    "blogger.com",
    "blogspot.com",
    "bloomberg.com",
    "blorbo.social",
    "boardgamegeek.com",
    "bodyandsoul.com",
    "boingboing.net",
    "bolha.social",
    "bordermail.com.au",
    "bostonglobe.com",
    "bostonglobe.ocm",
    "break.com",
    "breitbart.com",
    "brisbanetimes.com.au",
    "brw.com.au",
    "bsky.app",
    "businessinsider.com",
    "buttercupfestival.com",
    "buttersafe.com",
    "buzzfeed.com",
    "buzzfeednews.com",
    "cadence13.com",
    "camp.smolnet.org",
    "canberratimes.com.au",
    "cartonnetworkhq.com",
    "cartoonnetwork.com",
    "cbn.com",
    "cbs.com",
    "cbsnews.com",
    "cbssports.com",
    "cc.com",
    "character.ai",
    "chat-gpt.org",
    "christianitytoday.com",
    "cinemos.com",
    "clutchpoints.com",
    "cnbc.com",
    "cnet.com",
    "cnn.com",
    "coinbase.com",
    "coinmarketcap.com",
    "collegehumor.com",
    "comicbook.com",
    "commondreams.org",
    "community.nicfab.it",
    "computerfairi.es",
    "condenast.com",
    "consequence.net",
    "convo.casa",
    "cosmopolitan.com",
    "couriermail.com.au",
    "cpu-world.com",
    "cr8r.gg",
    "cracked.com",
    "crackle.com",
    "craigslist.org",
    "creativebloq.com",
    "creativereview.co.uk",
    "crinacle.com",
    "crunchyroll.com",
    "csmonitor.com",
    "cupoftea.social",
    "currentaffairs.org",
    "cwb.social",
    "dailyadvertiser.com.au",
    "dailycaller.com",
    "dailydot.com",
    "dailykos.com",
    "dailymail.co.uk",
    "dailymotion.com",
    "dailytelegraph.com.au",
    "dailywire.com",
    "dartreview.com",
    "deadline.com",
    "deeeep.io",
    "deezer.com",
    "denofgeek.com",
    "deseret.com",
    "designnews.com",
    "det.social",
    "deviantart.com",
    "dexerto.com",
    "diep.io",
    "diffen.com",
    "digg.com",
    "discovery.com",
    "discoveryplus.com",
    "disney.com",
    "disneynow.com",
    "disneyplus.com",
    "distractionware.com",
    "distrowatch.com",
    "donky.social",
    "dotesports.com",
    "douyin.com",
    "dpreview.com",
    "dribbble.com",
    "drop.com",
    "ebay.com",
    "economist.com",
    "elle.com",
    "engadget.com",
    "eorzea.photos",
    "espn.com",
    "esq.social",
    "esquire.com",
    "etsy.com",
    "evoke.ie",
    "expressional.social",
    "extra.ie",
    "extremetech.com",
    "facebook.com",
    "facebook.com/messages",
    "factiva.com",
    "faf.photos",
    "fair.org",
    "fandom.com",
    "fandom.ink",
    "fark.com",
    "feddit.de",
    "feddit.it",
    "fedibb.ml",
    "fedifilm.art",
    "fedipix.de",
    "firstthings.com",
    "fivethirtyeight.com",
    "flickr.com",
    "flipboard.com",
    "floatplane.com",
    "fnlondon.com",
    "fonearena.com",
    "foodandwine.com",
    "forbes.com",
    "foreignpolicy.com",
    "foros.fediverso.gal",
    "fortune.com",
    "fox.com",
    "foxnews.com",
    "foxsports.com",
    "foxsports.com.au",
    "foxtel.com.au",
    "freemalaysiatoday.com",
    "freespeech.org",
    "friendsofdesoto.social",
    "ft.com",
    "funnyordie.com",
    "fuzzy.directory",
    "gamersnexus.net",
    "gamesradar.com",
    "gazette.com",
    "gearspace.com",
    "geekdom.social",
    "geektyrant.com",
    "geohashing.site",
    "getpocket.com",
    "ghacks.net",
    "gigaom.com",
    "gizmodo.com",
    "glitchwave.com",
    "gocomics.com",
    "goldderby.com",
    "gossipcop.com",
    "gram.social",
    "group.lt",
    "gsmarena.com",
    "guru3d.com",
    "headphones.com",
    "heraldextra.com",
    "heraldsun.com.au",
    "hexus.net",
    "hiqarchitectura.com",
    "history.com",
    "hollywoodinsider.com",
    "hollywoodreporter.com",
    "homestuck.com",
    "horseandhound.co.uk",
    "hothardware.com",
    "howtogeek.com",
    "huffingtonpost.com",
    "huggingface.co",
    "hulu.com",
    "hydrogenaud.io",
    "hyperboleandahalf.blogspot.com",
    "icio.us",
    "ieji.de",
    "illawarramercury.com.au",
    "images.google.com",
    "imdb.com",
    "imgur.com",
    "indiatimes.com",
    "infidious.fdn.fr",
    "infowars.com",
    "insider.com",
    "insta.trom.tf",
    "instagram.com",
    "inthesetimes.com",
    "inv.riverside.rocks",
    "inv.skyn3t.in",
    "invidio.us",
    "invidio.xamh.de",
    "invidiou.site",
    "invidious.048596.xyz",
    "invidious.blamefran.net",
    "invidious.ethibox.fr",
    "invidious.exonip.de",
    "invidious.himiko.cloud",
    "invidious.hub.ne.kr",
    "invidious.io",
    "invidious.kavin.rocks",
    "invidious.moomoo.me",
    "invidious.namazso.eu",
    "invidious.noho.st",
    "invidious.reallyancient.tech",
    "invidious.s1gm4.eu",
    "invidious.silkky.cloud",
    "invidious.site",
    "invidious.synopta.org",
    "invidious.tinfoil-hat.net",
    "invidious.tube",
    "invidious.xyz",
    "invidious.zapashcanon.fr",
    "invidious.zee.li",
    "invidious-us.kavin.rocks",
    "iogames.space",
    "is.nota.live",
    "itch.io",
    "itsfoss.com",
    "jacobinmag.com",
    "jauntypix.net",
    "join-lemmy.org",
    "joinmastodon.org",
    "jspowerhour.com",
    "justfacts.com",
    "justfactsdaily.com",
    "kiwifarms.net",
    "knowyourmeme.com",
    "kongregate.com",
    "kotaku.com",
    "latimes.com",
    "learningdisability.social",
    "lem.simple-gear.com",
    "lemmy.blahaj.zone",
    "lemmy.ca",
    "lemmy.coupou.fr",
    "lemmy.eus",
    "lemmy.fediverse.jp",
    "lemmy.graz.social",
    "lemmy.helvetet.eu",
    "lemmy.ml",
    "lemmy.perthchat.org",
    "lemmy.pt",
    "lemmy.rimkus.it",
    "lemmy.rollenspiel.monster",
    "lemmy.schuerz.at",
    "lemmy.services.coupou.fr",
    "lemmy.toot.pt",
    "lemmygra.ml",
    "libretooth.gr",
    "life.com",
    "lifehacker.com",
    "lifenews.com",
    "lifewire.com",
    "liminal.southfox.me",
    "linkage.ds8.zone",
    "links.hackliberty.org",
    "links.roobre.es",
    "liveleak.com",
    "livescience.com",
    "lm.korako.me",
    "lonelyplanet.com",
    "looper.com",
    "lor.sh",
    "lounge.town",
    "macrumors.com",
    "mailonsunday.co.uk",
    "mailtoday.in",
    "makeuseof.com",
    "malaymail.com",
    "maly.io",
    "mander.xyz",
    "marketwatch.com",
    "mas.to",
    "mashable.com",
    "masr.social",
    "masto.ai",
    "masto.nu",
    "masto.yttrx.com",
    "mastodon.babb.be",
    "mastodon.online",
    "mastodon.podaboutli.st",
    "mastodon.sdf.org",
    "mastodon.social",
    "mastodon.world",
    "mastodon.xyz",
    "mediaite.com",
    "medium.com",
    "megatokyo.com",
    "mentalfloss.com",
    "metafilter.com",
    "metapixl.com",
    "metro.co.uk",
    "metro.news",
    "midjourney.com",
    "midwest.social",
    "militarytimes.com",
    "mindly.social",
    "minecraft.net",
    "miniature.photography",
    "miniclip.com",
    "miraheze.org",
    "mirror.co.uk",
    "mix.com",
    "mlb.com",
    "monero.house",
    "motherjones.com",
    "mountains.social",
    "msn.com",
    "msnbc.com",
    "mstdn.dk",
    "mstdn.social",
    "narratively.com",
    "natesilver.net",
    "natgeotv.com",
    "nationalgeographic.com",
    "nationalreview.com",
    "naturalnews.com",
    "nba.com",
    "nbc.com",
    "nbcnews.com",
    "ndtv.com",
    "neowin.net",
    "netflix.com",
    "newgrounds.com",
    "newrepublic.com",
    "news.co.uk",
    "news.com.au",
    "news.google.com",
    "news.ycombinator.com",
    "newsandguts.com",
    "newsbusters.org",
    "newscorpaustralia.com",
    "newsmax.com",
    "newsweek.com",
    "newsy.com",
    "newyorker.com",
    "nextpit.com",
    "nfl.com",
    "nhl.com",
    "nick.com",
    "nickjr.com",
    "nineentertainment.com.au",
    "nineentertainmentco.com.au",
    "nitter.1d4.us",
    "nitter.40two.app",
    "nitter.42l.fr",
    "nitter.actionsack.com",
    "nitter.bcow.xyz",
    "nitter.cattube.org",
    "nitter.cc",
    "nitter.dark.fail",
    "nitter.database.red",
    "nitter.domain.glass",
    "nitter.ethibox.fr",
    "nitter.eu",
    "nitter.exonip.de",
    "nitter.fdn.fr",
    "nitter.grimneko.de",
    "nitter.himiko.cloud",
    "nitter.hu",
    "nitter.jae.fi",
    "nitter.kavin.rocks",
    "nitter.koyu.space",
    "nitter.mailstation.de",
    "nitter.mha.fi",
    "nitter.moomoo.me",
    "nitter.namazso.eu",
    "nitter.net",
    "nitter.nixnet.services",
    "nitter.ortion.xyz",
    "nitter.pussthecat.org",
    "nitter.unixfox.eu",
    "nitter.vxempire.xyz",
    "nixorigin.one",
    "norcal.social",
    "notyoutube.org",
    "npr.org",
    "ntnews.com.au",
    "nydailynews.com",
    "nymag.com",
    "nypost.com",
    "nytimes.com",
    "oann.com",
    "occupydemocrats.com",
    "odysee.com",
    "oglaf.com",
    "ohai.social",
    "omegle.com",
    "omgubuntu.co.uk",
    "opalstack.social",
    "openai.com",
    "opensea.io",
    "ozy.com",
    "pagesix.com",
    "palmerreport.com",
    "paperio.com",
    "paramountplus.com",
    "pbfcomics.com",
    "pbs.org",
    "pcpartpicker.com",
    "peacocktv.com",
    "persians.life",
    "persiansmastodon.com",
    "petapixel.com",
    "phonearena.com",
    "phys.org",
    "pi.dead.guru",
    "pinboard.in",
    "pinterest.com",
    "pitchfork.com",
    "pix.anduin.net",
    "pixel.artemai.art",
    "pixel.mamutut.space",
    "pixel.tchncs.de",
    "pixelfed.au",
    "pixelfed.automat.click",
    "pixelfed.bachgau.social",
    "pixelfed.cafe",
    "pixelfed.cz",
    "pixelfed.de",
    "pixelfed.eus",
    "pixelfed.fioverse.zone",
    "pixelfed.fr",
    "pixelfed.hu",
    "pixelfed.nz",
    "pixelfed.org",
    "pixelfed.sg",
    "pixelfed.social",
    "pixelfed.tokyo",
    "pixels.gsi.li",
    "pixey.org",
    "pixtagram.social",
    "pjmedia.com",
    "playcanv.as",
    "playstation.com",
    "poal.co",
    "pointieststick.com",
    "pointlesssites.com",
    "poki.com",
    "politico.com",
    "politicususa.com",
    "popsci.com",
    "popular.info",
    "popularmechanics.com",
    "popurls.com",
    "prageru.com",
    "primarycare.app",
    "primelocation.com",
    "privagram.com",
    "producthunt.com",
    "professorwatchlist.org",
    "propublica.org",
    "pulse.ng",
    "pxlmo.com",
    "qobuz.com",
    "questionablecontent.net",
    "quillette.com",
    "quora.com",
    "qwantz.com",
    "radio.garden",
    "raphus.social",
    "rasmussenreports.com",
    "rateyourmusic.com",
    "rd.com",
    "readit.nsgn.eu",
    "readtangle.com",
    "readwrite.com",
    "realclearpolitics.com",
    "reason.com",
    "recode.net",
    "reddit.com",
    "redstate.com",
    "renkontu.com",
    "reuters.com",
    "roblox.com",
    "rockpapershotgun.com",
    "rollcall.com",
    "rottentomatoes.com",
    "rrrrthats5rs.com",
    "rt.com",
    "salon.com",
    "screencrush.com",
    "screenrant.com",
    "seventeen.com",
    "slant.co",
    "slashdot.org",
    "slate.com",
    "slatestarcodex.com",
    "sling.com",
    "slither.io",
    "slrpnk.net",
    "sltrib.com",
    "smbc-comics.com",
    "smh.com.au",
    "social.bau-ha.us",
    "social.vivaldi.net",
    "socialpixels.xyz",
    "sopuli.xyz",
    "soundcloud.com",
    "space.com",
    "spectator.org",
    "speedrun.com",
    "spiegel.de",
    "spindices.com",
    "splix.io",
    "sportskeeda.com",
    "spotify.com",
    "sputniknews.com",
    "sself.co",
    "stablediffusionweb.com",
    "stammtisch.hallertau.social",
    "standard.co.uk",
    "stranger.social",
    "stripes.com",
    "stuff.co.nz",
    "substack.com",
    "sundaystartimes.co.nz",
    "sunherald.com.au",
    "suntimes.com",
    "swisstoots.ch",
    "talkingpointsmemo.com",
    "tanx.io",
    "techcrunch.com",
    "techmeme.com",
    "techpowerup.com",
    "techradar.com",
    "techspot.com",
    "ted.com",
    "teenvogue.com",
    "tennis.com",
    "terrycavanaghgames.com",
    "terrysfreegameoftheweek.com",
    "theage.com.au",
    "theamericanconservative.com",
    "theatlantic.com",
    "theaustralian.com.au",
    "theblaze.com",
    "theblower.au",
    "thebulwark.com",
    "thecourier.com.au",
    "thedailybeast.com",
    "thedispatch.com",
    "theepochtimes.com",
    "thefederalist.com",
    "thefiscaltimes.com",
    "thegatewaypundit.com",
    "theguardian.com",
    "theherald.com.au",
    "thehill.com",
    "theland.com.au",
    "themarysue.com",
    "themercury.com.au",
    "thenation.com",
    "thenextweb.com",
    "theonion.com",
    "therealnews.com",
    "theregister.co.uk",
    "theregister.com",
    "thesandpaper.net",
    "thesun.co.uk",
    "the-sun.com",
    "thesundaytimes.co.uk",
    "thetimes.co.uk",
    "theverge.com",
    "theweek.com",
    "thisdaylive.com",
    "thisismoney.co.uk",
    "threads.net",
    "threewordphrase.com",
    "tictoc.social",
    "tiktok.com",
    "time.com",
    "tmz.com",
    "tomsguide.com",
    "tomshardware.com",
    "toot.community",
    "toot.funami.tech",
    "toot.io",
    "toot.site",
    "tooting.ch",
    "townhall.com",
    "tpointuk.co.uk",
    "tpusa.com",
    "treasurearena.com",
    "truthout.org",
    "tube.cadence.moe",
    "tube.connect.cafe",
    "tube.incog.host",
    "tubitv.com",
    "tumblr.com",
    "tvinsider.com",
    "tvtropes.org",
    "tweaktown.com",
    "tweet.lambda.dance",
    "twiiit.com",
    "twitch.tv",
    "twitchy.com",
    "twitit.gq",
    "twitr.gq",
    "twitter.censors.us",
    "twitter.com",
    "underconsideration.com",
    "universeodon.com",
    "unsplash.com",
    "upi.com",
    "urbandictionary.com",
    "usatoday.com",
    "userbenchmark.com",
    "vanityfair.com",
    "variety.com",
    "venturebeat.com",
    "versus.com",
    "vevo.com",
    "vice.com",
    "vid.mint.lgbt",
    "vid.puffyan.us",
    "videocardz.com",
    "vimeo.com",
    "vk.com",
    "voanews.com",
    "vogue.com",
    "vox.com",
    "vrv.co",
    "waitbutwhy.com",
    "washingtonexaminer.com",
    "washingtonmonthly.com",
    "washingtonpost.com",
    "washingtontimes.com",
    "watoday.com.au",
    "wattpad.com",
    "wccftech.com",
    "weather.com",
    "web.archive.org",
    "webtoons.com",
    "wegotthiscovered.com",
    "wikipedia.org",
    "wired.com",
    "wnd.com",
    "wondermark.com",
    "wonkette.com",
    "workingnotworking.com",
    "worldtruth.tv",
    "wowcher.co.uk",
    "wsj.com",
    "x.com",
    "x0r.be",
    "xbox.com",
    "xkcd.com",
    "y.com.cm",
    "yahoo.com",
    "yewtu.be",
    "yt.cyberhost.uk",
    "ytb.trom.tf",
    "ytprivate.com",
    "zdnet.com",
    "zombs.io",
    "zoopla.co.uk",
};

bool EndsWith(const std::string& host, const std::string& domain) {
  if (host == domain) {
    return true;
  }
  if (host.length() > domain.length() &&
      host[host.length() - domain.length() - 1] == '.' &&
      host.substr(host.length() - domain.length()) == domain) {
    return true;
  }
  return false;
}

}  // namespace

StudyModeNavigationThrottle::StudyModeNavigationThrottle(
    content::NavigationThrottleRegistry& registry)
    : content::NavigationThrottle(registry) {}

StudyModeNavigationThrottle::~StudyModeNavigationThrottle() = default;

const char* StudyModeNavigationThrottle::GetNameForLogging() {
  return "StudyModeNavigationThrottle";
}

// static
void StudyModeNavigationThrottle::MaybeCreateAndAdd(
    content::NavigationThrottleRegistry& registry) {
  if (g_study_mode_enabled) {
    registry.AddThrottle(
        std::make_unique<StudyModeNavigationThrottle>(registry));
  }
}

// static
void StudyModeNavigationThrottle::SetEnabled(bool enabled) {
  g_study_mode_enabled = enabled;
  LOG(INFO) << "NEOVEX Study Mode: " << (enabled ? "ENABLED" : "DISABLED");
}

// static
void StudyModeNavigationThrottle::SetWhitelist(
    const std::vector<std::string>& domains) {
  GetUserWhitelist() = domains;
}

// static
bool StudyModeNavigationThrottle::IsEnabled() {
  return g_study_mode_enabled;
}

// static
bool StudyModeNavigationThrottle::IsDomainBlocked(const std::string& host) {
  // Always allow youtube.com
  if (EndsWith(host, "youtube.com")) {
    return false;
  }

  // Check user whitelist
  for (const auto& allowed : GetUserWhitelist()) {
    if (EndsWith(host, allowed)) {
      return false;
    }
  }

  // Check blocked domains
  for (const char* domain : kBlockedDomains) {
    if (EndsWith(host, domain)) {
      return true;
    }
  }

  return false;
}

content::NavigationThrottle::ThrottleCheckResult
StudyModeNavigationThrottle::WillStartRequest() {
  return CheckNavigation();
}

content::NavigationThrottle::ThrottleCheckResult
StudyModeNavigationThrottle::WillRedirectRequest() {
  return CheckNavigation();
}

content::NavigationThrottle::ThrottleCheckResult
StudyModeNavigationThrottle::CheckNavigation() {
  const GURL& url = navigation_handle()->GetURL();

  const std::string host(url.host());

  // Check if this is our custom whitelist intercept URL
  if (host == "study-mode-whitelist.local") {
    std::string query(url.query());
    if (query.find("domain=") == 0) {
      std::string domain = query.substr(7);
      GetUserWhitelist().push_back(domain);
      LOG(INFO) << "NEOVEX Study Mode: Whitelisted " << domain;

      // Return a basic HTML page that auto-navigates back to the original page
      std::string back_html =
          "<!DOCTYPE html><html><head><title>Whitelisted</title></head>"
          "<body "
          "style=\"background:#0d0620;color:#fff;font-family:sans-serif;text-"
          "align:center;padding:50px;\">"
          "<h2>Whitelisted! Returning...</h2>"
          "<script>window.history.go(-2);</script>"
          "</body></html>";

      return content::NavigationThrottle::ThrottleCheckResult(
          content::NavigationThrottle::CANCEL, net::ERR_BLOCKED_BY_CLIENT,
          back_html);
    }
  }

  if (!url.SchemeIsHTTPOrHTTPS()) {
    return PROCEED;
  }
  if (IsDomainBlocked(host)) {
    LOG(INFO) << "NEOVEX Study Mode: Blocked " << host;

    std::string error_html = R"HTML(
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<style>
  body {
    background: linear-gradient(135deg, #0d0620 0%, #1a0b2e 50%, #0d0620 100%);
    color: #e8eaed;
    font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
    min-height: 100vh;
    display: flex;
    align-items: center;
    justify-content: center;
    margin: 0;
  }
  .container {
    background: rgba(30, 20, 50, 0.4);
    backdrop-filter: blur(12px);
    -webkit-backdrop-filter: blur(12px);
    border: 1px solid rgba(138, 110, 255, 0.2);
    border-radius: 20px;
    box-shadow: 0 8px 32px 0 rgba(0, 0, 0, 0.5);
    padding: 50px;
    text-align: center;
    max-width: 450px;
  }
  .icon {
    font-size: 64px;
    margin-bottom: 20px;
  }
  h1 {
    margin: 0 0 10px;
    font-size: 28px;
    font-weight: 600;
  }
  .domain {
    color: #a090c0;
    margin-bottom: 30px;
    font-size: 16px;
  }
  p {
    font-size: 15px;
    line-height: 1.5;
    margin-bottom: 30px;
    color: #d4bfff;
  }
  .buttons {
    display: flex;
    justify-content: center;
    gap: 15px;
  }
  button {
    padding: 12px 24px;
    border-radius: 12px;
    font-size: 15px;
    font-weight: 600;
    cursor: pointer;
    transition: all 0.2s ease;
    border: none;
  }
  .btn-primary {
    background: #60a5fa;
    color: #fff;
  }
  .btn-primary:hover {
    background: #3b82f6;
  }
  .btn-secondary {
    background: rgba(255, 255, 255, 0.1);
    color: #e8eaed;
    border: 1px solid rgba(255, 255, 255, 0.2);
  }
  .btn-secondary:hover {
    background: rgba(255, 255, 255, 0.2);
  }
</style>
</head>
<body>
  <div class="container">
    <div class="icon">🎓</div>
    <h1>Study Mode Active</h1>
    <div class="domain">)HTML" +
                             host + R"HTML(</div>
    <p>This site is blocked while Study Mode is enabled.<br><br>Stay focused &mdash; you've got this!</p>
    <div class="buttons">
      <button class="btn-primary" onclick="window.history.back()">Go Back</button>
      <button class="btn-secondary" onclick="window.location.href='http://study-mode-whitelist.local/?domain=)HTML" +
                             host + R"HTML('">Whitelist This Site</button>
    </div>
  </div>
</body>
</html>
)HTML";

    return content::NavigationThrottle::ThrottleCheckResult(
        content::NavigationThrottle::CANCEL, net::ERR_BLOCKED_BY_CLIENT,
        error_html);
  }

  return PROCEED;
}

// --- Neovex Shield Privacy Service ---

// static
NeovexShieldService* NeovexShieldService::GetInstance() {
  static base::NoDestructor<NeovexShieldService> instance;
  return instance.get();
}

NeovexShieldService::NeovexShieldService() {
  trackers_blocked_ = 0;
  ads_blocked_ = 0;
  fingerprints_blocked_ = 0;
  cookies_managed_ = 0;
}

void NeovexShieldService::IncrementTrackersBlocked() {
  trackers_blocked_++;
}
void NeovexShieldService::IncrementAdsBlocked() {
  ads_blocked_++;
}
void NeovexShieldService::IncrementFingerprintsBlocked() {
  fingerprints_blocked_++;
}
void NeovexShieldService::IncrementCookiesManaged() {
  cookies_managed_++;
}

int NeovexShieldService::GetTrackersBlocked() const {
  return trackers_blocked_.load();
}
int NeovexShieldService::GetAdsBlocked() const {
  return ads_blocked_.load();
}
int NeovexShieldService::GetFingerprintsBlocked() const {
  return fingerprints_blocked_.load();
}
int NeovexShieldService::GetCookiesManaged() const {
  return cookies_managed_.load();
}

// --- Neovex Shield Throttle ---

// static
bool NeovexShieldService::IsTrackerDomainForShield(const std::string& host) {
  const char* const kTrackers[] = {"google-analytics.com",
                                   "doubleclick.net",
                                   "facebook.net",
                                   "connect.facebook.net",
                                   "criteo.com",
                                   "googlesyndication.com",
                                   "scorecardresearch.com",
                                   "quantserve.com",
                                   "taboola.com",
                                   "outbrain.com",
                                   "adnxs.com",
                                   "adsrvr.org",
                                   "moatads.com",
                                   "amazon-adsystem.com",
                                   "rubiconproject.com",
                                   "casalemedia.com",
                                   "pubmatic.com",
                                   "crwdcntrl.net",
                                   "demdex.net",
                                   "ads-twitter.com",
                                   "rlcdn.com",
                                   "tapad.com",
                                   "advertising.com",
                                   "addthis.com",
                                   "adservice.google.com",
                                   "analytics.twitter.com"};
  for (const char* tracker : kTrackers) {
    if (base::EqualsCaseInsensitiveASCII(host, tracker)) {
      return true;
    }
    if (host.length() > std::string(tracker).length() &&
        host[host.length() - std::string(tracker).length() - 1] == '.' &&
        base::EndsWith(host, tracker, base::CompareCase::INSENSITIVE_ASCII)) {
      return true;
    }
  }
  return false;
}

// static
void NeovexShieldThrottle::MaybeCreateAndAdd(
    content::NavigationThrottleRegistry& registry) {
  registry.AddThrottle(std::make_unique<NeovexShieldThrottle>(registry));
}

NeovexShieldThrottle::NeovexShieldThrottle(
    content::NavigationThrottleRegistry& registry)
    : content::NavigationThrottle(registry) {}

NeovexShieldThrottle::~NeovexShieldThrottle() = default;

content::NavigationThrottle::ThrottleCheckResult
NeovexShieldThrottle::WillStartRequest() {
  return CheckIfBlocked();
}

content::NavigationThrottle::ThrottleCheckResult
NeovexShieldThrottle::WillRedirectRequest() {
  return CheckIfBlocked();
}

content::NavigationThrottle::ThrottleCheckResult
NeovexShieldThrottle::CheckIfBlocked() {
  const GURL& url = navigation_handle()->GetURL();
  std::string host = std::string(url.host());

  if (NeovexShieldService::IsTrackerDomainForShield(host)) {
    auto* shield = NeovexShieldService::GetInstance();
    shield->IncrementTrackersBlocked();

    if (host.find("doubleclick") != std::string::npos ||
        host.find("syndication") != std::string::npos ||
        host.find("ad") != std::string::npos) {
      shield->IncrementAdsBlocked();
    }

    // Simulate fingerprint attempts for specific heavy trackers
    if (host.find("crwdcntrl") != std::string::npos ||
        host.find("demdex") != std::string::npos) {
      shield->IncrementFingerprintsBlocked();
    }

    return content::NavigationThrottle::CANCEL;
  }
  return content::NavigationThrottle::PROCEED;
}

const char* NeovexShieldThrottle::GetNameForLogging() {
  return "NeovexShieldThrottle";
}

// --- Neovex Shield URLLoader Throttle ---

void NeovexShieldURLLoaderThrottle::WillStartRequest(
    network::ResourceRequest* request,
    bool* defer) {
  if (!request->url.SchemeIsHTTPOrHTTPS()) return;

  std::string host = std::string(request->url.host());
  if (NeovexShieldService::IsTrackerDomainForShield(host)) {
    auto* shield = NeovexShieldService::GetInstance();
    shield->IncrementTrackersBlocked();

    if (host.find("doubleclick") != std::string::npos ||
        host.find("syndication") != std::string::npos ||
        host.find("ad") != std::string::npos) {
      shield->IncrementAdsBlocked();
    }

    if (host.find("crwdcntrl") != std::string::npos ||
        host.find("demdex") != std::string::npos) {
      shield->IncrementFingerprintsBlocked();
    }

    delegate_->CancelWithError(net::ERR_BLOCKED_BY_CLIENT, "Neovex Shield Blocked Tracker");
  }
}

void NeovexShieldURLLoaderThrottle::WillRedirectRequest(
    net::RedirectInfo* redirect_info,
    const network::mojom::URLResponseHead& response_head,
    bool* defer,
    std::vector<std::string>* to_be_removed_request_headers,
    net::HttpRequestHeaders* modified_request_headers,
    net::HttpRequestHeaders* modified_cors_exempt_request_headers) {
  if (!redirect_info->new_url.SchemeIsHTTPOrHTTPS()) return;

  std::string host = std::string(redirect_info->new_url.host());
  if (NeovexShieldService::IsTrackerDomainForShield(host)) {
    auto* shield = NeovexShieldService::GetInstance();
    shield->IncrementTrackersBlocked();

    if (host.find("doubleclick") != std::string::npos ||
        host.find("syndication") != std::string::npos ||
        host.find("ad") != std::string::npos) {
      shield->IncrementAdsBlocked();
    }

    if (host.find("crwdcntrl") != std::string::npos ||
        host.find("demdex") != std::string::npos) {
      shield->IncrementFingerprintsBlocked();
    }

    delegate_->CancelWithError(net::ERR_BLOCKED_BY_CLIENT, "Neovex Shield Blocked Tracker");
  }
}
