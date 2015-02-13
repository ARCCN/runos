/*
 * Copyright 2015 Applied Research Center for Computer Networks
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

'use strict';
/* exported Color */
/* exported CTX */
/* exported HCI */
/* exported Net */
/* exported newHost */
/* exported newLink */
/* exported Rand */
/* exported Router */
/* exported UI */
/* globals Color:true */
/* globals CTX:true */
/* globals HCI:true */
/* globals Net:true */
/* globals newHost:true */
/* globals newLink:true */
/* globals Rand:true */
/* globals Router:true */
/* globals UI:true */
var Color = { pink: '#DD1772', darkblue: '#281A71', lightblue: '#0093DD', blue: '#1B7EE0', dblue: '#1560AB', lblue: '#1F90FF', red: '#E01B53', green: '#C6FF40' };
var CTX;
var HCI;
var Net;
var newHost;
var newLink;
var Rand = function (max, min) { min = min || (max -= 1, 0); return Math.floor(Math.random() * (max-min+1) + min); };
var UI = {};
