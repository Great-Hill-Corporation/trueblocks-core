/*-------------------------------------------------------------------------------------------
 * qblocks - fast, easily-accessible, fully-decentralized data from blockchains
 * copyright (c) 2018, 2019 TrueBlocks, LLC (http://trueblocks.io)
 *
 * This program is free software: you may redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version. This program is
 * distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even
 * the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details. You should have received a copy of the GNU General
 * Public License along with this program. If not, see http://www.gnu.org/licenses/.
 *-------------------------------------------------------------------------------------------*/
#include "options.h"
#include "handle_maker.h"

//-----------------------------------------------------------------------
int main(int argc, const char* argv[]) {
    etherlib_init(defaultQuitHandler);

    //    for (blknum_t bl = 3684349 ; bl < getBlockProgress(BP_CLIENT).client ; bl = bl + 5000)
    //        cout << bl << "\t" << bn_2_Date(bl).Format(FMT_JSON) << "\t" << wei_2_Ether(getUsdFromMakerAt(bl)) <<
    //        endl;

    COptions options;
    if (!options.prepareArguments(argc, argv))
        return 0;

    bool once = true;
    for (auto command : options.commandLines) {
        if (!options.parseArguments(command))
            return 0;

        string_q message;
        CPriceQuoteArray quotes;
        if (!loadPriceData(options.source, quotes, options.freshen, message) && quotes.size())
            return options.usage(message);

        if (once)
            cout << exportPreamble(expContext().fmtMap["header"], GETRUNTIME_CLASS(CPriceQuote));

        size_t step = (options.freq / 5);
        for (size_t i = 0; i < quotes.size(); i = i + step) {
            if (!visitPrice(quotes[i], &options))
                break;
        }
        once = false;
    }
    cout << exportPostamble(options.errors, expContext().fmtMap["meta"]);

    etherlib_cleanup();
    return 0;
}

//--------------------------------------------------------------
bool visitPrice(CPriceQuote& quote, void* data) {
    COptions* opt = reinterpret_cast<COptions*>(data);
    bool isText = (expContext().exportFmt & (TXT1 | CSV1));

    //////////////////////////////////////////////////////
    //////////////////////////////////////////////////////

    if (true) {
        if (isText) {
            cout << trim(quote.Format(expContext().fmtMap["format"]), '\t') << endl;

        } else {
            if (!opt->first)
                cout << ",";
            cout << "  ";
            indent();
            quote.toJson(cout);
            unindent();
            opt->first = false;
        }
    }

    if (isTestMode() && ts_2_Date(str_2_Ts(quote.Format("[{TIMESTAMP}]"))) >= time_q(2017, 8, 15, 0, 0, 0))
        return false;

    return !shouldQuit();
}
