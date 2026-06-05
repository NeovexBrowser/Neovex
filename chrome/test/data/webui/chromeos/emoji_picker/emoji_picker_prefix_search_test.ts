// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import {EmojiPrefixSearch} from 'neovex://emoji-picker/emoji_picker.js';
import {assertArrayEquals, assertEquals, assertFalse, assertTrue} from 'neovex://webui-test/chai_assert.js';

import {assertCloseTo} from './emoji_picker_test_util.js';

const mockCollection1 = [
  {
    base: {string: '😹', name: 'cat with tears of joy'},
    alternates: [],
  },
  {
    base: {string: '🤠', name: 'cowboy hat face'},
    alternates: [],
  },
  {
    base: {string: '🥲', name: 'smiling face with tear'},
    alternates: [],
  },
];

const mockCollection2 = [
  {
    base: {string: '😺', name: 'smiling cat'},
    alternates: [],
  },
  {
    base: {string: '😼', name: 'cat with wry smile'},
    alternates: [],
  },
  {
    base: {string: '🐛', name: 'caterpillar'},
    alternates: [],
  },
  {
    base: {string: '😇', name: 'smiling face with halo'},
    alternates: [],
  },
];

suite('PrefixSearchUnitTest', () => {
  let prefixSearch: EmojiPrefixSearch;
  setup(() => {
    prefixSearch = new EmojiPrefixSearch();
  });

  test('clear should erase all of the collection data.', () => {
    prefixSearch.setCollection(mockCollection1);
    prefixSearch.clear();

    const actualResults = prefixSearch.matchPrefixToEmojis('fa');

    assertEquals(0, actualResults.length);
  });

  test(
      'setCollection should replace the old collection with the new one.',
      () => {
        prefixSearch.setCollection(mockCollection1);
        prefixSearch.setCollection(mockCollection2);

        const actualResults = prefixSearch.matchPrefixToEmojis('smil');

        assertFalse(actualResults.includes('🥲'));
        assertTrue(actualResults.includes('😇'));
        assertTrue(actualResults.includes('😺'));
      });

  test('matchPrefixToEmojis should return the correct emojis.', () => {
    prefixSearch.setCollection(mockCollection1);

    const actualResults1 = prefixSearch.matchPrefixToEmojis('smil');
    const actualResults2 = prefixSearch.matchPrefixToEmojis('face');
    const actualResults3 = prefixSearch.matchPrefixToEmojis('tears');

    assertEquals(1, actualResults1.length);
    assertEquals('🥲', actualResults1[0]);
    assertEquals(2, actualResults2.length);
    assertTrue(actualResults2.includes('🤠'));
    assertTrue(actualResults2.includes('🥲'));
    assertEquals(1, actualResults3.length);
    assertTrue(actualResults3.includes('😹'));
    assertFalse(actualResults3.includes('🥲'));
  });

  test('Scoring single term against emoji using emoji name.', () => {
    const emojiRecord1 = mockCollection2[0]!;
    const emojiRecord2 = mockCollection2[1]!;
    const emojiRecord3 = mockCollection2[2]!;
    const emojiRecord4 = mockCollection2[3]!;

    assertCloseTo(
        prefixSearch.scoreTermAgainstEmoji(emojiRecord1, 'smil'), 4 / 7);
    assertCloseTo(prefixSearch.scoreTermAgainstEmoji(emojiRecord1, 'cat'), 0.5);
    assertCloseTo(
        prefixSearch.scoreTermAgainstEmoji(emojiRecord2, 'smil'), 0.2);
    assertCloseTo(prefixSearch.scoreTermAgainstEmoji(emojiRecord2, 'cat'), 1);
    assertEquals(prefixSearch.scoreTermAgainstEmoji(emojiRecord3, 'smil'), 0);
    assertEquals(
        prefixSearch.scoreTermAgainstEmoji(emojiRecord3, 'cat'), 3 / 11);
    assertEquals(
        prefixSearch.scoreTermAgainstEmoji(emojiRecord4, 'smil'), 4 / 7);
    assertEquals(prefixSearch.scoreTermAgainstEmoji(emojiRecord4, 'cat'), 0);
  });

  test(
      'Multi-word prefix search should return the correct emojis and scores.',
      () => {
        prefixSearch.setCollection(mockCollection2);

        const actualMatches = prefixSearch.search('smil cat');

        assertEquals(2, actualMatches.length);
        assertEquals(actualMatches[0]!.item.base.string, '😺');
        assertCloseTo(actualMatches[0]!.score, 16 / 77);
        assertEquals(actualMatches[1]!.item.base.string, '😼');
        assertCloseTo(actualMatches[1]!.score, 4 / 45);
      });

  test(
      'Order of prefix terms in the query should not affect search results.',
      () => {
        prefixSearch.setCollection(mockCollection2);

        assertArrayEquals(
            prefixSearch.search('smil cat'), prefixSearch.search('cat smil'));
        assertArrayEquals(
            prefixSearch.search('with smi'), prefixSearch.search('smi with'));
      });

  test(
      'Matches on longer parts of emoji name should rank higher than on ' +
          'shorter ones.',
      () => {
        prefixSearch.setCollection(mockCollection2);

        const actualMatches = prefixSearch.search('smiling');

        assertEquals(2, actualMatches.length);
        assertEquals(actualMatches[0]!.item.base.name, 'smiling cat');
        assertEquals(
            actualMatches[1]!.item.base.name, 'smiling face with halo');
      });

  test(
      'The case of the search query should not affect the results returned.',
      () => {
        prefixSearch.setCollection(mockCollection2);

        assertArrayEquals(
            prefixSearch.search('Smi With'), prefixSearch.search('sMI WITh'));
      });
});
